
#ifndef MATERIAL_HAIR_HLSL
#define MATERIAL_HAIR_HLSL

#include"common.hlsl"
#include"../debug.hlsl"
#include"../packing.hlsl"
#include"../intersection.hlsl"
#include"../static_sampler.hlsl"

#include"../debug.hlsl"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace hair{

///////////////////////////////////////////////////////////////////////////////////////////////////

float calc_phi(float3 dir, float3 v, float3 w)
{
	float tmp_phi = atan2(dot(w, dir), dot(v, dir));
	if(tmp_phi > 0){
		return tmp_phi;
	}else{
		return (2 * PI) + tmp_phi;
	}
}

float calc_rel_phi(float phi_i, float phi_o)
{
	float tmp_phi = abs(phi_o - phi_i);
	if(tmp_phi < PI){
		return tmp_phi;
	}else{
		return 2 * PI - tmp_phi;
	}
}

float calc_theta(float3 dir, float3 u)
{
	float cos = dot(u, dir);
	if(cos >= 1){
		return PI / 2;
	}else if(cos <= -1){
		return -PI / 2;
	}else{
		return (PI / 2) - acos(cos);
	}
}

float gaussian(float x, float a, float b)
{
	return exp(-pow2(x - a) / (2 * b * b));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace hair

///////////////////////////////////////////////////////////////////////////////////////////////////

struct hair_material
{
	float3	I_R;
	float	a_R;
	float	b_R;
	float3	I_TT;
	float	a_TT;
	float	b_TT;
	float	r_TT;
	float3	I_TRT;
	float	a_TRT;
	float	b_TRT;
	float3	I_g;
	float	r_g;
	float	phi_g;

	float	A_R;
	float	B_R;
	float	Z_R;

	float	A_TT;
	float	B_TT;
	float	C_TT;
	float	Z_TT;

	float	A_TRT;
	float	B_TRT;
	float	Z_TRT;

	float	C_g;
	float	D_g;
	float	Z_g;

	float	phi_o;
	float	theta_o;
	float3	u;
	float3	v;
	float3	w;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

hair_material load_hair_material(intersection isect, float3 wo, float lod = 0, uint2 dtid = 0)
{
	uint handle = isect.material_handle;
	float3 position = isect.position;
	float3 normal = isect.normal;
	float3 tangent = isect.tangent.xyz;
	float3 binormal = get_binormal(isect);
	float2 uv = isect.uv;

	ByteAddressBuffer buf = get_byteaddress_buffer(handle);
	uint4 data0 = buf.Load4(MATERIAL_HEADER_SIZE + 16 * 0);
	uint2 data1 = buf.Load2(MATERIAL_HEADER_SIZE + 16 * 1);

	float4 Ia_R = u8x4_unorm_to_f32x4(data0.x);
	float4 Ia_TT = u8x4_unorm_to_f32x4(data0.y);
	float4 Ia_TRT = u8x4_unorm_to_f32x4(data0.z);
	float4 Iphi_g = u8x4_unorm_to_f32x4(data0.w);
	float4 b = u8x4_unorm_to_f32x4(data1.x);
	float4 r = u8x4_unorm_to_f32x4(data1.y);

	hair_material mtl;
	mtl.I_R = Ia_R.rgb;
	mtl.a_R = Ia_R.a * (PI / 2);
	mtl.I_TT = Ia_TT.rgb;
	mtl.a_TT = Ia_TT.a * (PI / 2);
	mtl.I_TRT = Ia_TRT.rgb;
	mtl.a_TRT = Ia_TRT.a * (PI / 2);
	mtl.I_g = Iphi_g.rgb;
	mtl.phi_g = Iphi_g.a * (PI / 2);
	mtl.b_R = b[0] * (PI / 2);
	mtl.b_TT = b[1] * (PI / 2);
	mtl.b_TRT = b[2] * (PI / 2);
	mtl.r_TT = r[1] * (PI / 2);
	mtl.r_g = r[3] * (PI / 2);

	//tangent‚Æbinormal‚Ì‚Ç‚Á‚¿‚ð”¯‚Ì•ûŒü‚É‚·‚é‚©‘I‚×‚é‚æ‚¤‚É‚·‚éH
	//mtl.v = tangent;
	//mtl.u = binormal;
	//mtl.w = normal;
	mtl.v = normalize(cross(normal, float3(0,1,0)));
	mtl.w = normal;
	mtl.u = cross(mtl.v, mtl.w);

	mtl.phi_o = hair::calc_phi(wo, mtl.v, mtl.w);
	mtl.theta_o = hair::calc_theta(wo, mtl.u);
		
	mtl.A_R = atan((+PI * 0.25 + mtl.theta_o * 0.5 - mtl.a_R) / mtl.b_R);
	mtl.B_R = atan((-PI * 0.25 + mtl.theta_o * 0.5 - mtl.a_R) / mtl.b_R);
	mtl.Z_R = 1 / (8 * sqrt(2 * PI) * mtl.b_R);

	mtl.A_TT = atan((+PI * 0.25 + mtl.theta_o * 0.5 - mtl.a_TT) / mtl.b_TT);
	mtl.B_TT = atan((-PI * 0.25 + mtl.theta_o * 0.5 - mtl.a_TT) / mtl.b_TT);
	mtl.C_TT = 2 * atan(PI / mtl.r_TT);
	mtl.Z_TT = 1 / (4 * PI * mtl.b_TT * mtl.r_TT);

	mtl.A_TRT = atan((+PI * 0.25 + mtl.theta_o * 0.5 - mtl.a_TRT) / mtl.b_TRT);
	mtl.B_TRT = atan((-PI * 0.25 + mtl.theta_o * 0.5 - mtl.a_TRT) / mtl.b_TRT);
	mtl.Z_TRT = 1 / (8 * sqrt(2 * PI) * mtl.b_TRT);

	mtl.C_g = atan((PI * 1 - mtl.phi_g) / mtl.r_g);
	mtl.D_g = atan((PI * 0 - mtl.phi_g) / mtl.r_g);
	mtl.Z_g = 1 / (8 * PI * mtl.b_TRT * mtl.r_g);

	return mtl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool has_contribution(float nwi, hair_material mtl)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void calc_bsdf_pdf(float phi, float theta_i, hair_material mtl, out float3 diffuse, out float3 non_diffuse, out float pdf, uint2 dtid = 0)
{
	float theta_h = (mtl.theta_o + theta_i) / 2;
	float M_R = hair::gaussian(theta_h, mtl.a_R, mtl.b_R);
	float M_TT = hair::gaussian(theta_h, mtl.a_TT, mtl.b_TT);
	float M_TRT = hair::gaussian(theta_h, mtl.a_TRT, mtl.b_TRT);
	float N_R = cos(phi / 2);
	float N_TT = hair::gaussian(phi, PI, mtl.r_TT);
	float N_TRT = N_R;
	float N_g = hair::gaussian(phi, mtl.phi_g, mtl.r_g);

	diffuse = 0;
	non_diffuse = (
		mtl.I_R * (M_R * N_R * mtl.Z_R) + 
		mtl.I_TT * (M_TT * N_TT * mtl.Z_TT) + 
		mtl.I_TRT * (M_TRT * N_TRT * mtl.Z_TRT) + 
		mtl.I_TRT * mtl.I_g * (M_TRT * N_g * mtl.Z_g)
	);

	float cos_i = cos(theta_i);
	float pdf_theta_R = mtl.b_R / (2 * cos_i * (mtl.A_R - mtl.B_R) * (pow(theta_h - mtl.a_R, 2) + mtl.b_R * mtl.b_R));
	float pdf_theta_TT = mtl.b_TT / (2 * cos_i * (mtl.A_TT - mtl.B_TT) * (pow(theta_h - mtl.a_TT, 2) + mtl.b_TT * mtl.b_TT));
	float pdf_theta_TRT = mtl.b_TRT / (2 * cos_i * (mtl.A_TRT - mtl.B_TRT) * (pow(theta_h - mtl.a_TRT, 2) + mtl.b_TRT * mtl.b_TRT));
	float pdf_phi_R = cos(phi / 2) / 4;
	float pdf_phi_TT = mtl.r_TT / (mtl.C_TT * (pow(phi - PI, 2) + mtl.r_TT * mtl.r_TT));
	float pdf_phi_TRT = pdf_phi_R;
	float pdf_phi_g = mtl.r_g / (2 * (mtl.C_g - mtl.D_g) * (pow(phi - mtl.phi_g, 2) + mtl.r_g * mtl.r_g));

	float weight_R = luminance(mtl.I_R);
	float weight_g = luminance(mtl.I_g * mtl.I_TRT);
	float weight_TT = luminance(mtl.I_TT);
	float weight_TRT = luminance(mtl.I_TRT);

	pdf = (
		weight_R * pdf_theta_R * pdf_phi_R + 
		weight_TT * pdf_theta_TT * pdf_phi_TT + 
		weight_TRT * pdf_theta_TRT * pdf_phi_TRT + 
		weight_g * pdf_theta_TRT * pdf_phi_g) / (weight_R + weight_TT + weight_TRT + weight_g);
}

void calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, hair_material mtl, out float3 diffuse, out float3 non_diffuse, out float pdf, uint2 dtid = 0)
{
	float theta_i = hair::calc_theta(wi, mtl.u);
	float phi_i = hair::calc_phi(wi, mtl.v, mtl.w);
	float phi = hair::calc_rel_phi(phi_i, mtl.phi_o);
	calc_bsdf_pdf(phi, theta_i, mtl, diffuse, non_diffuse, pdf, dtid);
}

float4 calc_bsdf_pdf(float3 wo, float3 wi, float3 normal, hair_material mtl, uint2 dtid = 0)
{
	float pdf;
	float3 diffuse, non_diffuse;
	calc_bsdf_pdf(wo, wi, normal, mtl, diffuse, non_diffuse, pdf, dtid);
	return float4(diffuse + non_diffuse, pdf);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bsdf_sample sample_bsdf(float3 wo, float3 normal, hair_material mtl, float u0, float u1, uint2 dtid = 0)
{
	float weight_R = luminance(mtl.I_R);
	float weight_g = luminance(mtl.I_g * mtl.I_TRT);
	float weight_TT = luminance(mtl.I_TT);
	float weight_TRT = luminance(mtl.I_TRT);
	float inv_sum_weight = 1 / (weight_R + weight_g + weight_TT + weight_TRT);
	float pmf_R = weight_R * inv_sum_weight;
	float pmf_g = weight_g * inv_sum_weight;
	float pmf_TT = weight_TT * inv_sum_weight;
	float pmf_TRT = weight_TRT * inv_sum_weight;

	float phi, theta_i;
	if(u0 < pmf_R)
	{
		u0 /= pmf_R;
		phi = 2 * asin(2 * u0 - 1);
		theta_i = 2 * mtl.b_R * tan(u1 * (mtl.A_R - mtl.B_R) + mtl.B_R) + 2 * mtl.a_R - mtl.theta_o;
	}
	else if((u0 -= pmf_R) < pmf_TT)
	{
		u0 /= pmf_TT;
		phi = mtl.r_TT * tan(mtl.C_TT * (u0 - 0.5f)) + PI;
		theta_i = 2 * mtl.b_TT * tan(u1 * (mtl.A_TT - mtl.B_TT) + mtl.B_TT) + 2 * mtl.a_TT - mtl.theta_o;
		if(phi > PI){ phi -= 2 * PI; }
	}
	else if((u0 -= pmf_TT) < pmf_TRT)
	{
		u0 /= pmf_TRT;
		phi = 2 * asin(2 * u0 - 1);
		theta_i = 2 * mtl.b_TRT * tan(u1 * (mtl.A_TRT - mtl.B_TRT) + mtl.B_TRT) + 2 * mtl.a_TRT - mtl.theta_o;
	}
	else
	{
		float negate = (u1 < 0.5);
		if(u1 < 0.5)
			u1 *= 2;
		else
			u1 = (u1 - 0.5) * 2;

		u0 -= pmf_TRT;
		u0 /= pmf_g;
		phi = mtl.r_g * tan(u0 * (mtl.C_g - mtl.D_g) + mtl.D_g) + mtl.phi_g;
		theta_i = 2 * mtl.b_TRT * tan(u1 * (mtl.A_TRT - mtl.B_TRT) + mtl.B_TRT) + 2 * mtl.a_TRT - mtl.theta_o;
		if(negate){ phi = -phi; }
	}

	float phi_i = mtl.phi_o - phi;
	float sp = sin(phi_i);
	float cp = cos(phi_i);
	float st = sin(theta_i);
	float ct = cos(theta_i);

	bsdf_sample s;
	s.is_valid = true;
	s.w = mtl.v * (ct * cp) + mtl.w * (ct * sp) + mtl.u * (st);

	float3 diffuse, non_diffuse;
	calc_bsdf_pdf(abs(phi), theta_i, mtl, diffuse, non_diffuse, s.pdf, 0);
	//calc_bsdf_pdf(abs(phi), theta_i, mtl, diffuse, non_diffuse, s.pdf, dtid);

	float inv_pdf = 1 / s.pdf;
	s.diffuse_weight = diffuse * inv_pdf;
	s.non_diffuse_weight = non_diffuse * inv_pdf;
	s.weight = (diffuse + non_diffuse) * inv_pdf;

	return s;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
