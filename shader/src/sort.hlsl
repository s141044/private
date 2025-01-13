
#ifndef SORT_HLSL
#define SORT_HLSL

#include"root_constant.hlsl"

static const uint K = 5;			//1���[�v�ŏ�������r�b�g��.1<<K��32�𒴂�����_��
static const uint M = 16;			//1�X���b�h���������鐔.�ǂꂭ�炢���K�؂��������...
static const uint radix = 1u << K;	//�
static const uint num_threads = 256;
static const uint num_waves = num_threads / 32;
static const uint tile_size = num_threads * M;
static const uint num_loops = (32 + K - 1) / K; //�ő僋�[�v��

cbuffer sort_cb
{
	uint	num_elems;
	uint	num_tiles;
	uint2	reserved;
};

#if defined(GLOBAL_COUNT)

ByteAddressBuffer	src;
RWByteAddressBuffer	counter;

groupshared uint local_count[num_loops][radix + 1];

[numthreads(num_threads, 1, 1)]
void global_count(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
	if(gtid < radix)
	{
		for(uint i = 0; i < num_loops; i++)
			local_count[i][gtid] = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	for(uint i = 0; i < M; i++)
	{
		uint idx = num_threads * (M * gid + i) + gtid;
		if(idx >= num_elems)
			break;

		uint val = src.Load(4 * idx);
		for(uint j = 0; j < num_loops; j++)
		{
			uint val_j = (val >> (K * j)) & (radix - 1);
			InterlockedAdd(local_count[j][val_j], 1);
		}
	}
	GroupMemoryBarrierWithGroupSync();

	if(gtid < radix)
	{
		for(uint i = 0; i < num_loops; i++)
			counter.InterlockedAdd(4 * (radix * i + gtid), local_count[i][gtid]);
	}
}

#elif defined(GLOBAL_SCAN)

RWByteAddressBuffer	counter;
groupshared uint	local_count[(radix + 31) / 32];

[numthreads(radix, 1, 1)]
void global_scan(uint2 dtid : SV_DispatchThreadID)
{
	uint count = (dtid.x < radix) ? counter.Load(4 * (radix * dtid.y + dtid.x)) : 0;
	uint sum = WavePrefixSum(count);

	if(WaveGetLaneIndex() == 31)
		local_count[dtid.x / 32] = sum + count;
	GroupMemoryBarrierWithGroupSync();

	if(WaveGetLaneIndex() < (radix + 31) / 32)
	{
		uint sum = WavePrefixSum(local_count[dtid.x]);
		local_count[dtid.x] = sum;
	}
	GroupMemoryBarrierWithGroupSync();

	sum += local_count[dtid.x / 32];
	counter.Store(4 * (radix * dtid.y + dtid.x), sum);
}

#elif defined(REORDER)

ByteAddressBuffer	counter;
RWByteAddressBuffer	elems;
RWByteAddressBuffer	tile_info;
RWByteAddressBuffer	tile_index;
groupshared uint	local_tile;
groupshared uint	local_elems[tile_size];
groupshared uint	local_count[num_waves][radix + 1];

void wave_local_count_and_offset(uint val, out uint count, out uint offset)
{
	uint mask = 0xffffffff;
	for(uint k = 0; k < K; k++)
	{
		uint ballot = WaveActiveBallot((val >> k) & 0x1).x;
		mask &= ((WaveGetLaneIndex() >> k) & 0x1) ? ballot : ~ballot;
	}

	//�o�P�b�gi�ɑ�����v�f��(i�̓��[��index)
	count = countbits(mask);

	//val�Ɠ����o�P�b�g�ɑ����郌�[���̓��Ŏ��������Ԗڂ�
	mask = WaveReadLaneAt(mask, val);
	offset = countbits(mask & (0xffffffffu >> (31 - WaveGetLaneIndex()))) - 1;
}

uint wave_local_count(uint val)
{
	uint count, offset;
	wave_local_count_and_offset(val, count, offset);
	return count;
}

[numthreads(num_threads, 1, 1)]
void reorder(uint gtid : SV_GroupThreadID)
{
	uint tile;
	if(gtid == 0)
	{
		tile_index.InterlockedAdd(0, 1, tile);
		if(tile == num_tiles - 1){ tile_index.Store(0, 0); }
		local_tile = tile;
	}
	GroupMemoryBarrierWithGroupSync();
	tile = local_tile;
	
	uint x = gtid % radix;
	uint y = gtid / radix;
	for(uint i = y; i < num_waves; i += num_threads / radix)
		local_count[i][x] = 0;

	//���L�������ɏ�������
	for(uint i = 0; i < M; i++)
	{
		uint idx = num_threads * (M * tile + i) + gtid;
		uint val = (idx < num_elems) ? elems.Load(4 * idx) : 0xffffffff;
		local_elems[num_threads * i + gtid] = val;
	}
	GroupMemoryBarrierWithGroupSync();

	//���L����������ǂݍ��݁�wave���ŃJ�E���g
	uint vals[M];
	uint loop = root_constant;
	for(uint i = 0; i < M; i++)
	{
		vals[i] = local_elems[32 * (M * (gtid / 32) + i) + (gtid % 32)]; //1wave���A������32*M������.1�X���b�h��32�Ԋu��M������.
		uint count = wave_local_count((vals[i] >> (K * loop)) & (radix - 1));
		if(count > 0){ local_count[gtid / 32][WaveGetLaneIndex()] += count; }
	}
	GroupMemoryBarrierWithGroupSync();

	//wave�P�ʂ�prefixSum
	uint sum = 0;
	if(gtid < radix)
	{
		for(uint i = 0; i < num_waves; i++)
			sum += local_count[i][gtid];

		uint prev_sum = WavePrefixSum(sum); //(radix<32)������
		for(uint i = 0; i < num_waves; i++)
		{
			uint tmp = local_count[i][gtid];
			local_count[i][gtid] = prev_sum;
			prev_sum += tmp;
		}
	}
	GroupMemoryBarrierWithGroupSync();

	//���[�J���Ń\�[�g
	for(uint i = 0; i < M; i++)
	{
		uint count, offset;
		uint bucket_idx = (vals[i] >> (K * loop)) & (radix - 1);
		uint tmp_offset = local_count[gtid / 32][bucket_idx];
		wave_local_count_and_offset(bucket_idx, count, offset);
		if(count > 0){ local_count[gtid / 32][WaveGetLaneIndex()] += count; }
		local_elems[offset + tmp_offset] = vals[i];
	}
	GroupMemoryBarrierWithGroupSync();

	//�O���[�o���ȃI�t�Z�b�g���v�Z
	if(gtid < radix)
	{
		uint tile_count_mask			= 0x3fffffff;
		uint tile_info_mask				= 0xc0000000;
		uint tile_local_sum_ready_flag	= 0x40000000;
		uint tile_global_sum_ready_flag = (loop % 2 == 0) ? 0x80000000 : 0x00000000;

		uint local_offset = WavePrefixSum(sum); //(radix<32)������
		uint global_offset = 0;
		if(tile == 0)
		{
			global_offset = counter.Load(4 * (radix * loop + gtid));

			uint original_value;
			//tile_info.Store(4 * (radix * tile + gtid), global_offset | tile_global_sum_ready_flag);
			tile_info.InterlockedExchange(4 * (radix * tile + gtid), (global_offset + sum) | tile_global_sum_ready_flag, original_value); //�O�̂���Atomic�ɏ�������
			local_count[0][gtid] = global_offset - local_offset; //�����o�����̃C���f�b�N�X��(i - localPrefixSum[bucket]) + globalPrefixSum[bucket]
		}
		else
		{
			uint original_value;
			//tile_info.Store(4 * (radix * tile + gtid), global_offset | tile_local_sum_ready_flag);
			tile_info.InterlockedExchange(4 * (radix * tile + gtid), sum | tile_local_sum_ready_flag, original_value); //�O�̂���Atomic�ɏ�������

			uint i = tile - 1;
			while(true)
			{
				uint info;
				//info = tile_info.Load(4 * (radix * i + gtid)); //Load����œK���̂������������[�v
				tile_info.InterlockedXor(4 * (radix * i + gtid), 0, info);

				if((info & tile_info_mask) == tile_global_sum_ready_flag)
				{
					global_offset += info & tile_count_mask;
					tile_info.Store(4 * (radix * tile + gtid), (global_offset + sum) | tile_global_sum_ready_flag);
					local_count[0][gtid] = global_offset - local_offset; //�����o�����̃C���f�b�N�X��(i - localPrefixSum[bucket]) + globalPrefixSum[bucket]
					break;
				}
				else if((info & tile_info_mask) == tile_local_sum_ready_flag)
				{
					//�߂肷���Ȃ��悤��
					if(tile - i < 16)
					{
						global_offset += info & tile_count_mask;
						i--;
					}
				}
			}
		}
	}
	GroupMemoryBarrierWithGroupSync();

	//�����o��
	for(uint i = gtid; i < tile_size; i += num_threads)
	{
		uint val = local_elems[i];
		uint bucket_idx = (val >> (K * loop)) & (radix - 1);
		uint idx = i + local_count[0][bucket_idx];
		if(idx >= num_elems){ break; }
		elems.Store(4 * idx, val);
	}
}

#endif
#endif