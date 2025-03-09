
#ifndef EMISSIVE_HLSL
#define EMISSIVE_HLSL

#include"utility.hlsl"
#include"packing.hlsl"
#include"raytracing.hlsl"
#include"root_constant.hlsl"

cbuffer emissive_blas_cbuf
{
	uint primitive_count;
	uint raytracing_instance_index;
	uint bindless_instance_index;
	float power;
};

uint to_fixed_point(float x, float max_x)
{
	//primitive_countは最大0xffff/3なので,0x7fff回加算できればいい
	//最大値を0x20000にすれば0x7fff回加算できる(最大値はfloat表現できる値(2のべき乗)にする方がいいはず)
	return uint((x / max_x) * 0x20000 + 0.5f);
}

float to_floating_point(uint x, float max_x)
{
	return (x / float(0x20000)) * max_x;
}

#if defined(INITIALIZE)

RWByteAddressBuffer	weight_uav;
RWByteAddressBuffer	max_weight_uav;
groupshared float	shared_weight[8];

[numthreads(256, 1, 1)]
void initialize(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID)
{
	float weight = 0;
	if(dtid < primitive_count)
	{
		bindless_instance_desc instance = bindless_instance_descs[bindless_instance_index];
		geometry_desc geom = load_geometry_desc(instance.bindless_geometry_handle);

		uint3 index = load_index(geom.ib_handle, instance.start_index_location + 3 * dtid);
		index += instance.base_vertex_location;

		ByteAddressBuffer vb0 = get_byteaddress_buffer(geom.vb_handles[0]);
		float3 p0 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.x));
		float3 p1 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.y));
		float3 p2 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.z));
		float area = length(cross(p1 - p0, p2 - p0)) / 2;
		if(area > 0){ weight = area; }
		weight_uav.Store<float>(4 * dtid, weight);
	}

	weight = WaveActiveMax(weight);
	if(WaveIsFirstLane())
		shared_weight[gtid / 32] = weight;
	GroupMemoryBarrierWithGroupSync();

	if(gtid < 8)
	{
		weight = WaveActiveMax(shared_weight[gtid]);
		if(gtid == 0)
			max_weight_uav.InterlockedMax(0, asuint(weight));
	}
}

#elif defined(INITIALIZE2)

ByteAddressBuffer	weight_srv;
ByteAddressBuffer	max_weight_srv;
RWByteAddressBuffer	sum_weight_uav;
groupshared uint	shared_weight[8];

[numthreads(256, 1, 1)]
void initialize2(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID)
{
	uint weight = 0;
	if(dtid < primitive_count)
		weight = to_fixed_point(weight_srv.Load<float>(4 * dtid), max_weight_srv.Load<float>(0));

	weight = WaveActiveSum(weight);
	if(WaveIsFirstLane())
		shared_weight[gtid / 32] = weight;
	GroupMemoryBarrierWithGroupSync();

	if(gtid < 8)
	{
		weight = WaveActiveSum(shared_weight[gtid]);
		if(gtid == 0)
			sum_weight_uav.InterlockedAdd(0, weight);
	}
}

#elif defined(SCAN1)

ByteAddressBuffer	weight_srv;
ByteAddressBuffer	max_weight_srv;
ByteAddressBuffer	sum_weight_srv;
RWByteAddressBuffer	block_light_count_uav;
RWByteAddressBuffer	block_heavy_count_uav;
RWByteAddressBuffer	block_light_sum_weight_uav;
RWByteAddressBuffer	block_heavy_sum_weight_uav;
groupshared uint	shared_light_count[8];
groupshared uint	shared_heavy_count[8];
groupshared uint	shared_light_sum_weight[8];
groupshared uint	shared_heavy_sum_weight[8];

[numthreads(256, 1, 1)]
void scan1(uint dtid : SV_DispatchThreadID, uint gid : SV_GroupID, uint group_index : SV_GroupIndex)
{
	uint weight = 0;
	if(dtid < primitive_count)
		weight = to_fixed_point(weight_srv.Load<float>(4 * dtid), max_weight_srv.Load<float>(0));

	bool is_heavy = (weight * primitive_count >= sum_weight_srv.Load(0));
	bool is_light = (dtid < primitive_count) && !is_heavy;

	uint light_count = WaveActiveCountBits(is_light);
	uint heavy_count = WaveActiveCountBits(is_heavy);
	uint light_sum_weight = WaveActiveSum(is_light ? weight : 0);
	uint heavy_sum_weight = WaveActiveSum(is_heavy ? weight : 0);

	if((group_index % 32) == 0)
	{
		shared_light_count[group_index / 32] = light_count;
		shared_heavy_count[group_index / 32] = heavy_count;
		shared_light_sum_weight[group_index / 32] = light_sum_weight;
		shared_heavy_sum_weight[group_index / 32] = heavy_sum_weight;
	}
	GroupMemoryBarrierWithGroupSync();

	if(group_index < 8)
	{
		light_count = WaveActiveSum(shared_light_count[group_index]);
		heavy_count = WaveActiveSum(shared_heavy_count[group_index]);
		light_sum_weight = WaveActiveSum(shared_light_sum_weight[group_index]);
		heavy_sum_weight = WaveActiveSum(shared_heavy_sum_weight[group_index]);

		block_light_count_uav.Store(4 * gid, light_count);
		block_heavy_count_uav.Store(4 * gid, heavy_count);
		block_light_sum_weight_uav.Store(4 * gid, light_sum_weight);
		block_heavy_sum_weight_uav.Store(4 * gid, heavy_sum_weight);
	}
}

#elif defined(SCAN2)

RWByteAddressBuffer	block_light_count_uav;
RWByteAddressBuffer	block_heavy_count_uav;
RWByteAddressBuffer	block_light_sum_weight_uav;
RWByteAddressBuffer	block_heavy_sum_weight_uav;
groupshared uint	shared_light_count[8];
groupshared uint	shared_heavy_count[8];
groupshared uint	shared_light_sum_weight[8];
groupshared uint	shared_heavy_sum_weight[8];

[numthreads(256, 1, 1)]
void scan2(uint dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
	uint light_count = 0;
	uint heavy_count = 0;
	uint light_sum_weight = 0;
	uint heavy_sum_weight = 0;
	if(dtid < (primitive_count + 255) / 256)
	{
		light_count = block_light_count_uav.Load(4 * dtid);
		heavy_count = block_heavy_count_uav.Load(4 * dtid);
		light_sum_weight = block_light_sum_weight_uav.Load(4 * dtid);
		heavy_sum_weight = block_heavy_sum_weight_uav.Load(4 * dtid);
	}
	
	uint sum_light_count = WavePrefixSum(light_count);
	uint sum_heavy_count = WavePrefixSum(heavy_count);
	uint sum_light_sum_weight = WavePrefixSum(light_sum_weight);
	uint sum_heavy_sum_weight = WavePrefixSum(heavy_sum_weight);

	if((group_index % 32) == 31)
	{
		shared_light_count[group_index / 32] = sum_light_count + light_count;
		shared_heavy_count[group_index / 32] = sum_heavy_count + heavy_count;
		shared_light_sum_weight[group_index / 32] = sum_light_sum_weight + light_sum_weight;
		shared_heavy_sum_weight[group_index / 32] = sum_heavy_sum_weight + heavy_sum_weight;
	}
	GroupMemoryBarrierWithGroupSync();

	if(group_index < 8)
	{
		shared_light_count[group_index] = WavePrefixSum(shared_light_count[group_index]);
		shared_heavy_count[group_index] = WavePrefixSum(shared_heavy_count[group_index]);
		shared_light_sum_weight[group_index] = WavePrefixSum(shared_light_sum_weight[group_index]);
		shared_heavy_sum_weight[group_index] = WavePrefixSum(shared_heavy_sum_weight[group_index]);
	}
	GroupMemoryBarrierWithGroupSync();
	
	if(dtid < (primitive_count + 255) / 256 + 1)
	{
		block_light_count_uav.Store(4 * dtid, shared_light_count[group_index / 32] + sum_light_count);
		block_heavy_count_uav.Store(4 * dtid, shared_heavy_count[group_index / 32] + sum_heavy_count);
		block_light_sum_weight_uav.Store(4 * dtid, shared_light_sum_weight[group_index / 32] + sum_light_sum_weight);
		block_heavy_sum_weight_uav.Store(4 * dtid, shared_heavy_sum_weight[group_index / 32] + sum_heavy_sum_weight);
	}
}

#elif defined(SCAN3)

ByteAddressBuffer	weight_srv;
ByteAddressBuffer	max_weight_srv;
ByteAddressBuffer	sum_weight_srv;
ByteAddressBuffer	block_light_count_srv;
ByteAddressBuffer	block_heavy_count_srv;
ByteAddressBuffer	block_light_sum_weight_srv;
ByteAddressBuffer	block_heavy_sum_weight_srv;
RWByteAddressBuffer	light_sum_weight_uav;
RWByteAddressBuffer	heavy_sum_weight_uav;
RWByteAddressBuffer	light_index_uav;
RWByteAddressBuffer	heavy_index_uav;
groupshared uint	shared_light_count[9];
groupshared uint	shared_heavy_count[9];
groupshared uint	shared_light_sum_weight[9];
groupshared uint	shared_heavy_sum_weight[9];
groupshared uint	shared_index[256];
groupshared uint	shared_weight[256];

[numthreads(256, 1, 1)]
void scan3(uint dtid : SV_DispatchThreadID, uint gid : SV_GroupID, uint group_index : SV_GroupIndex)
{
	float max_weight = max_weight_srv.Load<float>(0);

	uint weight = 0;
	if(dtid < primitive_count)
		weight = to_fixed_point(weight_srv.Load<float>(4 * dtid), max_weight);

	bool is_heavy = (weight * primitive_count >= sum_weight_srv.Load(0));
	bool is_light = (dtid < primitive_count) && !is_heavy;

	uint light_count = WavePrefixCountBits(is_light);
	uint heavy_count = WavePrefixCountBits(is_heavy);
	uint light_sum_weight = WavePrefixSum(is_light ? weight : 0);
	uint heavy_sum_weight = WavePrefixSum(is_heavy ? weight : 0);

	if(is_light)
		light_sum_weight += weight;
	else
		heavy_sum_weight += weight;

	if((group_index % 32) == 31)
	{
		shared_light_count[group_index / 32] = light_count + (is_light ? 1 : 0);
		shared_heavy_count[group_index / 32] = heavy_count + (is_heavy ? 1 : 0);
		shared_light_sum_weight[group_index / 32] = light_sum_weight;
		shared_heavy_sum_weight[group_index / 32] = heavy_sum_weight;
	}
	GroupMemoryBarrierWithGroupSync();

	if(group_index < 9)
	{
		shared_light_count[group_index] = WavePrefixSum(shared_light_count[group_index]);
		shared_heavy_count[group_index] = WavePrefixSum(shared_heavy_count[group_index]);
		shared_light_sum_weight[group_index] = WavePrefixSum(shared_light_sum_weight[group_index]);
		shared_heavy_sum_weight[group_index] = WavePrefixSum(shared_heavy_sum_weight[group_index]);
		shared_light_sum_weight[group_index] += block_light_sum_weight_srv.Load(4 * gid);
		shared_heavy_sum_weight[group_index] += block_heavy_sum_weight_srv.Load(4 * gid);
	}
	GroupMemoryBarrierWithGroupSync();

	uint block_light_count = shared_light_count[8];
	uint block_heavy_count = shared_heavy_count[8];

	light_count += shared_light_count[group_index / 32];
	heavy_count += shared_heavy_count[group_index / 32];
	light_sum_weight += shared_light_sum_weight[group_index / 32];
	heavy_sum_weight += shared_heavy_sum_weight[group_index / 32];

	if(is_light)
	{
		shared_index[light_count] = dtid;
		shared_weight[light_count] = light_sum_weight;
	}
	else if(is_heavy)
	{
		shared_index[block_light_count + heavy_count] = dtid;
		shared_weight[block_light_count + heavy_count] = heavy_sum_weight;
	}
	GroupMemoryBarrierWithGroupSync();

	uint global_light_offset = block_light_count_srv.Load(4 * gid);
	uint global_heavy_offset = block_heavy_count_srv.Load(4 * gid);

	//build時にthreshold=1になるように調整
	float W = to_floating_point(sum_weight_srv.Load(0), max_weight);
	float inv_threshold = primitive_count / W;
	if(group_index < block_light_count)
	{
		light_index_uav.Store(4 * (global_light_offset + group_index), shared_index[group_index]);
		light_sum_weight_uav.Store<float>(4 * (global_light_offset + group_index + 1), to_floating_point(shared_weight[group_index], max_weight) * inv_threshold);
	}
	if(group_index < block_heavy_count)
	{
		heavy_index_uav.Store(4 * (global_heavy_offset + group_index), shared_index[block_light_count + group_index]);
		heavy_sum_weight_uav.Store<float>(4 * (global_heavy_offset + group_index + 1), to_floating_point(shared_weight[block_light_count + group_index], max_weight) * inv_threshold);
	}
}

#elif defined(SCAN)

ByteAddressBuffer	weight_srv;
ByteAddressBuffer	max_weight_srv;
ByteAddressBuffer	sum_weight_srv;
RWByteAddressBuffer	light_sum_weight_uav;
RWByteAddressBuffer	heavy_sum_weight_uav;
RWByteAddressBuffer	light_index_uav;
RWByteAddressBuffer	heavy_index_uav;
RWByteAddressBuffer	scan_scratch_uav;
groupshared uint	shared_light_count[9];
groupshared uint	shared_heavy_count[9];
groupshared uint	shared_light_sum_weight[9];
groupshared uint	shared_heavy_sum_weight[9];
groupshared uint	shared_index[256];
groupshared uint	shared_weight[256];
groupshared uint	global_light_offset;
groupshared uint	global_heavy_offset;

[numthreads(256, 1, 1)]
void scan(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID)
{
	float max_weight = max_weight_srv.Load<float>(0);

	uint weight = 0;
	if(dtid < primitive_count)
		weight = to_fixed_point(weight_srv.Load<float>(4 * dtid), max_weight);

	bool is_heavy = (weight * primitive_count >= sum_weight_srv.Load(0));
	bool is_light = (dtid < primitive_count) && !is_heavy;

	uint light_count = WavePrefixCountBits(is_light);
	uint heavy_count = WavePrefixCountBits(is_heavy);
	uint light_sum_weight = WavePrefixSum(is_light ? weight : 0);
	uint heavy_sum_weight = WavePrefixSum(is_heavy ? weight : 0);

	if(is_light)
		light_sum_weight += weight;
	else
		heavy_sum_weight += weight;

	if((gtid % 32) == 31)
	{
		shared_light_count[gtid / 32] = light_count + (is_light ? 1 : 0);
		shared_heavy_count[gtid / 32] = heavy_count + (is_heavy ? 1 : 0);
		shared_light_sum_weight[gtid / 32] = light_sum_weight;
		shared_heavy_sum_weight[gtid / 32] = heavy_sum_weight;
	}
	GroupMemoryBarrierWithGroupSync();

	if(gtid < 9)
	{
		shared_light_count[gtid] = WavePrefixSum(shared_light_count[gtid]);
		shared_heavy_count[gtid] = WavePrefixSum(shared_heavy_count[gtid]);
		shared_light_sum_weight[gtid] = WavePrefixSum(shared_light_sum_weight[gtid]);
		shared_heavy_sum_weight[gtid] = WavePrefixSum(shared_heavy_sum_weight[gtid]);

		uint global_light_sum_weight;
		uint global_heavy_sum_weight;
		if(gtid == 8)
		{
			//countとweightはInterlockedAddが同時に行う必要がある

			uint64_t original;
			scan_scratch_uav.InterlockedAdd64(0, uint64_t(shared_light_count[8]) | (uint64_t(shared_light_sum_weight[8]) << 32), original);
			global_light_offset = uint(original & 0xffffffff);
			global_light_sum_weight = uint(original >> 32);

			scan_scratch_uav.InterlockedAdd64(8, uint64_t(shared_heavy_count[8]) | (uint64_t(shared_heavy_sum_weight[8]) << 32), original);
			global_heavy_offset = uint(original & 0xffffffff);
			global_heavy_sum_weight = uint(original >> 32);
		}
		shared_light_sum_weight[gtid] += WaveReadLaneAt(global_light_sum_weight, 8);
		shared_heavy_sum_weight[gtid] += WaveReadLaneAt(global_heavy_sum_weight, 8);
	}
	GroupMemoryBarrierWithGroupSync();

	uint block_light_count = shared_light_count[8];
	uint block_heavy_count = shared_heavy_count[8];

	light_count += shared_light_count[gtid / 32];
	heavy_count += shared_heavy_count[gtid / 32];
	light_sum_weight += shared_light_sum_weight[gtid / 32];
	heavy_sum_weight += shared_heavy_sum_weight[gtid / 32];

	if(dtid < primitive_count)
	{
		if(is_light)
		{
			shared_index[light_count] = dtid;
			shared_weight[light_count] = light_sum_weight;
		}
		else if(is_heavy)
		{
			shared_index[block_light_count + heavy_count] = dtid;
			shared_weight[block_light_count + heavy_count] = heavy_sum_weight;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	//build時にthreshold=1になるように調整
	float W = to_floating_point(sum_weight_srv.Load(0), max_weight);
	float inv_threshold = primitive_count / W;
	if(gtid < block_light_count)
	{
		light_index_uav.Store(4 * (global_light_offset + gtid), shared_index[gtid]);
		light_sum_weight_uav.Store<float>(4 * (global_light_offset + gtid + 1), to_floating_point(shared_weight[gtid], max_weight) * inv_threshold);
	}
	if(gtid < block_heavy_count)
	{
		heavy_index_uav.Store(4 * (global_heavy_offset + gtid), shared_index[block_light_count + gtid]);
		heavy_sum_weight_uav.Store<float>(4 * (global_heavy_offset + gtid + 1), to_floating_point(shared_weight[block_light_count + gtid], max_weight) * inv_threshold);
	}
}

#elif defined(BUILD)

RWByteAddressBuffer	blas_uav;
RWByteAddressBuffer	debug_uav;
ByteAddressBuffer	weight_srv;
ByteAddressBuffer	sum_weight_srv;
ByteAddressBuffer	max_weight_srv;
ByteAddressBuffer	light_sum_weight_srv;
ByteAddressBuffer	heavy_sum_weight_srv;
ByteAddressBuffer	light_index_srv;
ByteAddressBuffer	heavy_index_srv;

ByteAddressBuffer	scan_scratch_srv;

ByteAddressBuffer	block_light_count_srv;
ByteAddressBuffer	block_heavy_count_srv;

struct state
{
	uint i, j;
	float spill;
};

[numthreads(256, 1, 1)]
void build(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID)
{
	if(dtid >= primitive_count)
		return;

	{
		float w = weight_srv.Load<float>(4 * dtid);
		float W = to_floating_point(sum_weight_srv.Load(0), max_weight_srv.Load<float>(0));
		float threshold = W / primitive_count;
		debug_uav.Store<float>(4 * dtid, w / threshold);
	}

	uint light_count = scan_scratch_srv.Load(0);
	uint heavy_count = scan_scratch_srv.Load(8);
	//uint light_count = block_light_count_srv.Load(4 * ((primitive_count + 255) / 256));
	//uint heavy_count = block_heavy_count_srv.Load(4 * ((primitive_count + 255) / 256));

	state state;
	if(dtid == 0)
	{
		state.i = 0;
		state.j = 0;
		state.spill = heavy_sum_weight_srv.Load<float>(4);
	}
	else if(dtid == 1)
	{
		state.i = 1;
		state.j = 0;
		state.spill = heavy_sum_weight_srv.Load<float>(4) - (1 - light_sum_weight_srv.Load<float>(4));
	}
	else if(dtid == primitive_count - 1)
	{
		state.i = light_count;
		state.j = heavy_count - 1;
		state.spill = 1;
	}
	else
	{
		uint n = dtid;
		//uint a = 0;
		//uint b = n - 1;
		uint a = (n > light_count) ? n - light_count : 0;
		uint b = min(n, heavy_count) - 1;

		//while(true)
		for(uint k = 0; k < 16; k++)
		{
			uint j = (a + b) / 2;
			uint i = n - j;

			//範囲外はsumが0になってるのでいる
			//if(j > heavy_count)
			//{
			//	b = j - 1;
			//	continue;
			//}
			//if(i > light_count)
			//{
			//	a = j - 1;
			//	continue;
			//}

			//lightが使われないことはない
			//if(i == 0)
			//{
			//	a = j - 1;
			//	continue;
			//}

			float light_sum_weight = light_sum_weight_srv.Load<float>(4 * i);
			float heavy_sum_weight = heavy_sum_weight_srv.Load<float>(4 * j);
			float sigma1 = light_sum_weight + heavy_sum_weight;
			if(sigma1 <= n)
			{
				float heavy_sum_weight_next = heavy_sum_weight_srv.Load<float>(4 * (j + 1));
				float sigma2 = light_sum_weight + heavy_sum_weight_next;
				if(sigma2 > n)
				{
					//i-1,j+1では上の2つの条件を満たしていないのを確認
					//heavyの方が重みが大きいためsigma2の方の条件は確定で満たすのでsigma1の条件のみチェック

					float light_sum_weight_prev = light_sum_weight_srv.Load<float>(4 * (i - 1));
					float sigma1_ = light_sum_weight_prev + heavy_sum_weight_next;
					if(!(sigma1_ <= n))
					{
						state.i = i;
						state.j = j;
						state.spill = sigma2 - n;
						break;
					}
				}
				a = j + 1;
			}
			else
			{
				b = j - 1;
			}
		}
	}

	uint heavy_index = heavy_index_srv.Load(4 * state.j);
	if(state.spill > 1)
	{
		uint light_index = light_index_srv.Load(4 * state.i);
		float2 light_weights = light_sum_weight_srv.Load<float2>(4 * state.i);
		blas_uav.Store2(8 * light_index, uint2(asuint(light_weights[1] - light_weights[0]), heavy_index));
	}
	else
	{
		uint next_heavy_index = heavy_index_srv.Load(4 * (state.j + 1));
		blas_uav.Store2(8 * heavy_index, uint2(asuint(state.spill), next_heavy_index));
	}
}

//#elif defined(BUILD_TLAS)
//
//ByteAddressBuffer	emissive_instance_list_srv;
//RWByteAddressBuffer	emissive_instance_list_uav;
//RWByteAddressBuffer	emissive_instance_list_size_uav;
//groupshared	float	shared_power[8];
//groupshared	uint	shared_offset[8];
//
//[numthreads(256, 1, 1)]
//void build_tlas(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupThreadID)
//{
//}

#endif
#endif