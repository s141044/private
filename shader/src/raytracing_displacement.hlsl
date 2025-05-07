
#ifndef RAY_TRACING_DISPLACEMENT_HLSL
#define RAY_TRACING_DISPLACEMENT_HLSL

#include"math.hlsl"
#include"mesh.hlsl"
#include"debug.hlsl"
#include"utility.hlsl"
#include"raytracing.hlsl"
#include"displacement.hlsl"
#include"static_sampler.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 calc_barycentrics(float3 s, float3 c0, float3 c1, float3 c2)
{
	float3 x0 = c1 - c0;
	float3 x1 = c2 - c0;
	float3 x2 =  s - c0;
	float d00 = dot(x0, x0);
	float d01 = dot(x0, x1);
	float d11 = dot(x1, x1);
	float d20 = dot(x2, x0);
	float d21 = dot(x2, x1);
	float inv_denom = 1 / (d00 * d11 - d01 * d01);

	float3 b;
	b.y = (d11 * d20 - d01 * d21) * inv_denom;
	b.z = (d00 * d21 - d01 * d20) * inv_denom;
	b.x = 1 - b.y - b.z;
	return b;
}

bool intersect(float3 v0, float3 v1, float3 v2, float3 o, float3 d, float tmin, float tmax, inout float t, inout float b1, inout float b2)
{
	float3 e1 = v1 - v0;
	float3 e2 = v2 - v0;

	float3 r = o - v0;
	float3 s = cross(d, e2);
	float3 u = cross(r, e1);

	float inv_det = 1 / dot(s, e1);

	t = dot(u, e2) * inv_det;
	if((t <= tmin) || (tmax <= t))
		return false;

	b1 = dot(s, r) * inv_det;
	if(b1 < 0)
		return false;

	b2 = dot(u, d) * inv_det;
	if(b2 < 0)
		return false;

	return (b1 + b2 <= 1);
}

bool intersect(float3 q00, float3 q10, float3 q11, float3 q01, float3 o, float3 d, float tmin, float tmax, inout float t)
{
	float3 e10 = q10 - q00;
	float3 e11 = q11 - q10;
	float3 e00 = q01 - q00;
	float3 qn = cross(q10 - q00, q01 - q11);
	q00 -= o;
	q10 -= o;

	float a = dot(cross(q00, d), e00);
	float c = dot(qn, d);
	float b = dot(cross(q10, d), e11);
	b -= a + c;

	float det = b * b - 4 * a * c;
	if(det < 0)
		return false;
	
	det = sqrt(det);

	float u1, u2;
	if(c == 0)
	{
		u1 = -a / b;
		u2 = -1;
	}
	else
	{
		u1 = (-b - copysign(det, b)) / 2;
		u2 = a / u1;
		u1 /= c;
	}

	t = tmax;

	float u, v;
	if((0 <= u1) && (u1 <= 1))
	{
		float3 pa = lerp(q00, q10, u1);
		float3 pb = lerp(e00, e11, u1);
		float3 n = cross(d, pb);
		det = dot(n, n);
		n = cross(n, pa);

		float t1 = dot(n, pb);
		float v1 = dot(n, d);
		if((t1 > tmin * det) && (0 <= v1) && (v1 <= det))
		{
			t = t1 / det;
			u = u1; 
			v = v1 / det;
		}
	}
	if ((0 <= u2) && (u2 <= 1))
	{
		float3 pa = lerp(q00, q10, u2);
		float3 pb = lerp(e00, e11, u2);
		float3 n = cross(d, pb);
		det = dot(n, n);
		n = cross(n, pa);

		float t2 = dot(n, pb) / det;
		float v2 = dot(n, d);
		if((0 <= v2) && (v2 <= det) && (t > t2) && (t2 > tmin))
		{
			t = t2;
			u = u2;
			v = v2 / det;
		}
	}

	return (t != tmax);
}

bool triangle_intersection(uint instance_index, uint instance_id, uint geometry_index, uint primitive_index, float3 ray_o, float3 ray_d, inout float ray_t, float ray_tmin, float ray_tmax, inout float2 barycentrics, inout bool is_front_face)
{
	bindless_instance_desc instance = bindless_instance_descs[instance_id + geometry_index];
	geometry_desc geom = load_geometry_desc(instance.bindless_geometry_handle);

	uint3 index = load_index(geom.ib_handle, instance.start_index_location + 3 * primitive_index);
	index += instance.base_vertex_location;

	ByteAddressBuffer mtl_buf = get_byteaddress_buffer(instance.bindless_material_handle);
	uint2 mtl_vals = mtl_buf.Load2(0);

	Texture2D<float> displacement_map = get_texture2d<float>(mtl_vals[0]);
	float max_displacement = mtl_vals[1];

	ByteAddressBuffer vb0 = get_byteaddress_buffer(geom.vb_handles[0]);
	ByteAddressBuffer vb1 = get_byteaddress_buffer(geom.vb_handles[1]);
	ByteAddressBuffer vb2 = get_byteaddress_buffer(geom.vb_handles[2]);

	float3 v0 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.x));
	float3 v1 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.y));
	float3 v2 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.z));
	
	if(!intersect(v0, v1, v2, ray_o, ray_d, ray_tmin, ray_tmax, ray_t, barycentrics.x, barycentrics.y))
		return false;

	is_front_face = (dot(cross(v1 - v0, v2 - v0), ray_d) < 0);
	return true;
}

bool displacement_intersection(uint instance_index, uint instance_id, uint geometry_index, uint primitive_index, float3 ray_o, float3 ray_d, inout float ray_t, float ray_tmin, float ray_tmax, inout float2 barycentrics, inout bool is_front_face, uint2 dtid = 0)
{
	bindless_instance_desc instance = bindless_instance_descs[instance_id + geometry_index];
	geometry_desc geom = load_geometry_desc(instance.bindless_geometry_handle);

	uint3 index = load_index(geom.ib_handle, instance.start_index_location + 3 * primitive_index);
	index += instance.base_vertex_location;

	ByteAddressBuffer mtl_buf = get_byteaddress_buffer(instance.bindless_material_handle);
	displacement_params params = load_displacement_params(mtl_buf);

	ByteAddressBuffer vb0 = get_byteaddress_buffer(geom.vb_handles[0]);
	ByteAddressBuffer vb1 = get_byteaddress_buffer(geom.vb_handles[1]);
	ByteAddressBuffer vb2 = get_byteaddress_buffer(geom.vb_handles[2]);

	float3 v0 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.x));
	float3 v1 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.y));
	float3 v2 = asfloat(vb0.Load3(geom.offsets[0] + (geom.strides[0] & 0xff) * index.z));
	float3 ng = normalize(cross(v1 - v0, v2 - v0));
	
	uint enc_n0 = vb1.Load(geom.offsets[1] + ((geom.strides[0] >> 8) & 0xff) * index.x);
	uint enc_n1 = vb1.Load(geom.offsets[1] + ((geom.strides[0] >> 8) & 0xff) * index.y);
	uint enc_n2 = vb1.Load(geom.offsets[1] + ((geom.strides[0] >> 8) & 0xff) * index.z);
	
	float2 uv0 = asfloat(vb2.Load2(geom.offsets[2] + ((geom.strides[0] >> 16) & 0xff) * index.x + 4));
	float2 uv1 = asfloat(vb2.Load2(geom.offsets[2] + ((geom.strides[0] >> 16) & 0xff) * index.y + 4));
	float2 uv2 = asfloat(vb2.Load2(geom.offsets[2] + ((geom.strides[0] >> 16) & 0xff) * index.z + 4));
	
	float3 e0 = v0 + decode_direction(enc_n0) * params.max_displacement;
	float3 e1 = v1 + decode_direction(enc_n1) * params.max_displacement;
	float3 e2 = v2 + decode_direction(enc_n2) * params.max_displacement;
	
	float tmin = +FLT_MAX;
	float tmax = -FLT_MAX;

	float t, b1, b2;
	if(intersect(v0, v1, v2, ray_o, ray_d, ray_tmin, FLT_MAX, t, b1, b2))
	{
		tmin = min(tmin, t);
		tmax = max(tmax, t);
	}
	if(intersect(e0, e1, e2, ray_o, ray_d, ray_tmin, FLT_MAX, t, b1, b2))
	{
		tmin = min(tmin, t);
		tmax = max(tmax, t);
	}
	if(intersect(v0, v1, e1, e0, ray_o, ray_d, ray_tmin, FLT_MAX, t))
	{
		tmin = min(tmin, t);
		tmax = max(tmax, t);
	}
	if(intersect(v1, v2, e2, e1, ray_o, ray_d, ray_tmin, FLT_MAX, t))
	{
		tmin = min(tmin, t);
		tmax = max(tmax, t);
	}
	if(intersect(v2, v0, e0, e2, ray_o, ray_d, ray_tmin, FLT_MAX, t))
	{
		tmin = min(tmin, t);
		tmax = max(tmax, t);
	}

	if(tmin == FLT_MAX)
		return false;

	if(tmin >= ray_tmax)
		return false;

	//“à•”
	if(tmin == tmax)
		tmin = ray_tmin;

	tmax = min(tmax, ray_tmax);

	t = tmin;
	float3 s = ray_o + ray_d * t;

	float3 n0 = decode_direction(enc_n0);
	float3 n1 = decode_direction(enc_n1);
	float3 n2 = decode_direction(enc_n2);

	float3 inv_ng_n;
	inv_ng_n[0] = 1 / dot(ng, n0);
	inv_ng_n[1] = 1 / dot(ng, n1);
	inv_ng_n[2] = 1 / dot(ng, n2);

	float dh = dot(ng, s - v0);
	float3 c0 = v0 + n0 * inv_ng_n[0] * dh;
	float3 c1 = v1 + n1 * inv_ng_n[1] * dh;
	float3 c2 = v2 + n2 * inv_ng_n[2] * dh;

	float3 b = calc_barycentrics(s, c0, c1, c2);
	float3 p = v0 * b[0] + v1 * b[1] + v2 * b[2];
	float3 n = n0 * b[0] + n1 * b[1] + n2 * b[2];
	float2 uv = uv0 * b[0] + uv1 * b[1] + uv2 * b[2];

	float h_ray = dot(s - p, s - p);
	float h_surf = pow2(get_displacement(params, uv)) * dot(n, n);
	float h_diff = h_surf - h_ray;
	bool initial_result = (h_ray < h_surf);

	float dt = 0.001f; //TODO: ‚©‚µ‚±‚­

	while(true)
	{
		t += dt;
		s = ray_o + ray_d * min(tmax, t);

		dh = dot(ng, s - v0);
		c0 = v0 + n0 * inv_ng_n[0] * dh;
		c1 = v1 + n1 * inv_ng_n[1] * dh;
		c2 = v2 + n2 * inv_ng_n[2] * dh;

		b = calc_barycentrics(s, c0, c1, c2);
		p = v0 * b[0] + v1 * b[1] + v2 * b[2];
		n = n0 * b[0] + n1 * b[1] + n2 * b[2];
		uv = uv0 * b[0] + uv1 * b[1] + uv2 * b[2];

		h_ray = dot(s - p, s - p);
		h_surf = pow2(get_displacement(params, uv)) * dot(n, n);

		if((h_ray < h_surf) != initial_result)
			break;

		if(t >= tmax)
			return false;

		h_diff = h_surf - h_ray;
	}

	if(t > tmax)
	{
		dt -= (t - tmax);
		t = tmax;
	}

	float u = (h_surf - h_ray) / (h_surf - h_ray - h_diff);
	s -= ray_d * dt * u;
	t -= dt * u;

	dh = dot(ng, s - v0);
	c0 = v0 + n0 * dh;
	c1 = v1 + n1 * dh;
	c2 = v2 + n2 * dh;

	b = calc_barycentrics(s, c0, c1, c2);

	ray_t = t;
	barycentrics = b.yz;
	is_front_face = initial_result ? 0 : 1;
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
