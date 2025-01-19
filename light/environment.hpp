
#pragma once

#ifndef NN_RENDER_LIGHT_ENVIRONMENT_HPP
#define NN_RENDER_LIGHT_ENVIRONMENT_HPP

#include"../image.hpp"
#include"../core_impl.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//environment_light
///////////////////////////////////////////////////////////////////////////////////////////////////

class environment_light
{
public:

	//コンストラクタ
	environment_light(const hdr_image &hdri, const uint size)
	{
		assert(std::has_single_bit(size));

		m_shader_file = gp_shader_manager->create(L"environment.sdf.json");

		std::vector<uint> src(hdri.width() * hdri.height());
		for(int y = hdri.height() - 1, i = 0; y >= 0; y--)
		{
			for(int x = 0; x < hdri.width(); x++)
			{
				const float3 L = r8g8b8e8_to_f32x3(hdri(x, y));
				src[i++] = f32x3_to_r9g9b9e5(L);
			}
		}
		mp_src_tex = gp_render_device->create_texture2d(texture_format_r9g9b9e5_sharedexp, hdri.width(), hdri.height(), 1, resource_flag_allow_shader_resource, src.data());
		mp_src_srv = gp_render_device->create_shader_resource_view(*mp_src_tex, texture_srv_desc(*mp_src_tex));
		gp_render_device->set_name(*mp_src_tex, L"environment_light: src_tex");

		const uint mip_levels = std::countr_zero(size) + 1;
		mp_cube_tex = gp_render_device->create_texture_cube(texture_format_r9g9b9e5_sharedexp, size, size, mip_levels, resource_flag_allow_shader_resource);
		gp_render_device->set_name(*mp_cube_tex, L"environment_light: cube_tex");
	}

	//初期化
	bool initialize(render_context &context)
	{
		if(m_shader_file.has_update())
		{
			m_shader_file.update();
			mp_cube_srv = nullptr;
		}
		else if(m_shader_file.is_invalid())
			return false;
		if(mp_cube_srv != nullptr)
			return true;

		const uint w = mp_cube_tex->width();
		const uint h = mp_cube_tex->height();
		const uint n = mp_cube_tex->mip_levels();

		//r9g9b9e5のuavを作れないのでr32uintに書いてからコピーする
		auto p_tmp_tex = gp_render_device->create_texture2d_array(texture_format_r32_uint, w, h, 6, n, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access | resource_flag_scratch));
		gp_render_device->set_name(*p_tmp_tex, L"environment_light: tmp_tex");

		unordered_access_view_ptr uav_ptrs[16];
		for(uint i = 0; i < n; i++)
		{
			uav_ptrs[i] = gp_render_device->create_unordered_access_view(*p_tmp_tex, texture_uav_desc(*p_tmp_tex, i));
		}

		sampler_desc sampler_desc = {};
		sampler_desc.filter = filter_type_min_mag_mip_linear;
		sampler_desc.address_u = address_mode_wrap;
		sampler_desc.address_v = address_mode_clamp;
		sampler_desc.address_w = address_mode_clamp;
		auto p_sampler = gp_render_device->create_sampler(sampler_desc);

		context.set_priority(priority_initiaize);
		context.set_pipeline_resource("src", *mp_src_srv);
		context.set_pipeline_resource("dst", *uav_ptrs[0]);
		context.set_pipeline_resource("panorama_sampler", *p_sampler);
		context.set_pipeline_state(*m_shader_file.get("initialize_lod0"));
		context.dispatch_with_32bit_constant(ceil_div(w, 16), ceil_div(h, 16), 6, reinterpret<uint>(1 / float(w)));

		for(uint i = 1; i < n; i += 6)
		{
			auto p_srv = gp_render_device->create_shader_resource_view(*p_tmp_tex, texture_srv_desc(*p_tmp_tex, i - 1));
			context.set_pipeline_resource("src", *p_srv);
			context.set_pipeline_resource("dst1", *uav_ptrs[i + 0]);
			context.set_pipeline_resource("dst2", *uav_ptrs[i + 1]);
			context.set_pipeline_resource("dst3", *uav_ptrs[i + 2]);
			context.set_pipeline_resource("dst4", *uav_ptrs[i + 3]);
			context.set_pipeline_resource("dst5", *uav_ptrs[i + 4]);
			context.set_pipeline_resource("dst6", *uav_ptrs[i + 5]);

			switch(std::min(n - i, 6u)){
			case 1: context.set_pipeline_state(*m_shader_file.get("generate_mipmap1")); break;
			case 2: context.set_pipeline_state(*m_shader_file.get("generate_mipmap2")); break;
			case 3: context.set_pipeline_state(*m_shader_file.get("generate_mipmap3")); break;
			case 4: context.set_pipeline_state(*m_shader_file.get("generate_mipmap4")); break;
			case 5: context.set_pipeline_state(*m_shader_file.get("generate_mipmap5")); break;
			case 6: context.set_pipeline_state(*m_shader_file.get("generate_mipmap6")); break;
			}
			context.dispatch_with_32bit_constant(ceil_div(w >> i, 32), ceil_div(h >> i, 32), 6, reinterpret<uint>(1 / float(w >> (i - 1))));
		}

		//r32uint->r9g9b9e5
		context.copy_texture(*mp_cube_tex, *p_tmp_tex);

		mp_cube_srv = gp_render_device->create_shader_resource_view(*mp_cube_tex, texture_srv_desc(*mp_cube_tex));
		return true;
	}

	//テクスチャ/SRVを返す
	texture &src_tex() const { return *mp_src_tex; }
	texture &cube_tex() const { return *mp_cube_tex; }
	shader_resource_view &src_srv() const { return *mp_src_srv; }
	shader_resource_view &cube_srv() const { return *mp_cube_srv; }

private:

	float3 r8g8b8e8_to_f32x3(const unsigned char rgbe[4])
	{
		const unsigned char r = rgbe[0];
		const unsigned char g = rgbe[1];
		const unsigned char b = rgbe[2];
		const unsigned char e = rgbe[3];
		if(e == 0)
			return float3(0, 0, 0);
		
		const auto scale = scalbn(1.0f, e - (128 + 8));
		return float3(r, g, b) * scale;
	}

private:

	texture_ptr					mp_src_tex;
	texture_ptr					mp_cube_tex;
	shader_resource_view_ptr	mp_src_srv;
	shader_resource_view_ptr	mp_cube_srv;
	shader_file_holder			m_shader_file;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//environment_light_sampler
///////////////////////////////////////////////////////////////////////////////////////////////////

class environment_light_sampler
{
public:

	//コンストラクタ
	environment_light_sampler() : mp_cube_tex()
	{
		m_shader_file = gp_shader_manager->create(L"environment_sampling.sdf.json");
		mp_unnormalized_cdf_buf = gp_render_device->create_byteaddress_buffer(sizeof(float) * 6, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
		mp_unnormalized_cdf_srv = gp_render_device->create_shader_resource_view(*mp_unnormalized_cdf_buf, buffer_srv_desc(*mp_unnormalized_cdf_buf));
		mp_unnormalized_cdf_uav = gp_render_device->create_unordered_access_view(*mp_unnormalized_cdf_buf, buffer_uav_desc(*mp_unnormalized_cdf_buf));
	}

	bool initialize(render_context &context, environment_light &envmap, uint res = 64, uint sample_count = 16 * 1024)
	{
		assert(std::has_single_bit(res));
		assert(std::has_single_bit(sample_count));

		if(m_shader_file.has_update())
		{
			m_shader_file.update();
			mp_cube_tex = nullptr;
		}
		else if(m_shader_file.is_invalid())
			return false;

		auto &cube_tex = envmap.cube_tex();
		auto &cube_srv = envmap.cube_srv();
		res = std::min(res, cube_tex.width());

		if((&cube_tex != mp_cube_tex) || (mp_weight_tex == nullptr) || (mp_weight_tex->width() != res))
		{
			const uint n = std::countr_zero(res) + 1;
			mp_weight_tex = gp_render_device->create_texture2d_array(texture_format_r32_float, res, res, 6, n, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			for(uint i = 0; i < n; i++){ mp_weight_srv[i] = gp_render_device->create_shader_resource_view(*mp_weight_tex, texture_srv_desc(*mp_weight_tex, i, -1)); }
			gp_render_device->set_name(*mp_weight_tex, L"environment_light_sampler: weight_tex");

			unordered_access_view_ptr uav_ptrs[16];
			for(uint i = 0; i < n; i++){ uav_ptrs[i] = gp_render_device->create_unordered_access_view(*mp_weight_tex, texture_uav_desc(*mp_weight_tex, i)); }

			context.set_pipeline_resource("src", cube_srv);
			context.set_pipeline_resource("dst", *uav_ptrs[0]);
			context.set_pipeline_state(*m_shader_file.get("initialize_weight"));
			context.dispatch_with_32bit_constant(ceil_div(res, 16), ceil_div(res, 16), 6, (cube_tex.mip_levels() - n) | (res << 16));

			for(uint i = 1; i < n; i += 6)
			{
				context.set_pipeline_resource("src", *mp_weight_srv[i - 1]);
				context.set_pipeline_resource("dst1", *uav_ptrs[i + 0]);
				context.set_pipeline_resource("dst2", *uav_ptrs[i + 1]);
				context.set_pipeline_resource("dst3", *uav_ptrs[i + 2]);
				context.set_pipeline_resource("dst4", *uav_ptrs[i + 3]);
				context.set_pipeline_resource("dst5", *uav_ptrs[i + 4]);
				context.set_pipeline_resource("dst6", *uav_ptrs[i + 5]);
			
				switch(std::min(cube_tex.mip_levels() - i, 6u)){
				case 1: context.set_pipeline_state(*m_shader_file.get("generate_mipmap1")); break;
				case 2: context.set_pipeline_state(*m_shader_file.get("generate_mipmap2")); break;
				case 3: context.set_pipeline_state(*m_shader_file.get("generate_mipmap3")); break;
				case 4: context.set_pipeline_state(*m_shader_file.get("generate_mipmap4")); break;
				case 5: context.set_pipeline_state(*m_shader_file.get("generate_mipmap5")); break;
				case 6: context.set_pipeline_state(*m_shader_file.get("generate_mipmap6")); break;
				}
				context.dispatch_with_32bit_constant(ceil_div(res >> i, 32), ceil_div(res >> i, 32), 6, reinterpret<uint>(1 / float(res >> (i - 1))));
			}

			context.set_pipeline_resource("src", *mp_weight_srv[n - 1]);
			context.set_pipeline_resource("dst", *mp_unnormalized_cdf_uav);
			context.set_pipeline_state(*m_shader_file.get("calculate_cdf"));
			context.dispatch(1, 1, 1);
			mp_cube_tex = &cube_tex;

			if((mp_sample_buf == nullptr) || (mp_sample_buf->num_elements() != sample_count))
			{
				struct sample
				{
					uint	power;
					uint	w;
					float	pdf;
				};
				mp_sample_buf = gp_render_device->create_structured_buffer(sizeof(sample), sample_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
				mp_sample_srv = gp_render_device->create_shader_resource_view(*mp_sample_buf, buffer_srv_desc(*mp_sample_buf));
				mp_sample_uav = gp_render_device->create_unordered_access_view(*mp_sample_buf, buffer_uav_desc(*mp_sample_buf));
			}

			bind(context);
			context.set_pipeline_resource("envmap", cube_srv);
			context.set_pipeline_resource("samples", *mp_sample_uav);
			context.set_pipeline_state(*m_shader_file.get("presample"));
			context.dispatch_with_32bit_constant(ceil_div(sample_count, 256), 1, 1, (cube_tex.mip_levels() - mp_weight_tex->mip_levels()) | (sample_count << 16));
		}
		return true;
	}

	//バインド
	void bind(render_context &context) const
	{
		context.set_pipeline_resource("envmap_cdf", *mp_unnormalized_cdf_srv);
		context.set_pipeline_resource("envmap_weight", *mp_weight_srv[0]);
		context.set_pipeline_resource("envmap_samples", *mp_sample_srv);

		const auto mip_levels = int(mp_weight_tex->mip_levels());
		context.set_pipeline_resource("envmap_weight1", *mp_weight_srv[std::max(mip_levels - 2, 0)]);
		context.set_pipeline_resource("envmap_weight2", *mp_weight_srv[std::max(mip_levels - 3, 0)]);
		context.set_pipeline_resource("envmap_weight3", *mp_weight_srv[std::max(mip_levels - 4, 0)]);
		context.set_pipeline_resource("envmap_weight4", *mp_weight_srv[std::max(mip_levels - 5, 0)]);
		context.set_pipeline_resource("envmap_weight5", *mp_weight_srv[std::max(mip_levels - 6, 0)]);
		context.set_pipeline_resource("envmap_weight6", *mp_weight_srv[std::max(mip_levels - 7, 0)]);
		context.set_pipeline_resource("envmap_weight7", *mp_weight_srv[std::max(mip_levels - 8, 0)]);
		context.set_pipeline_resource("envmap_weight8", *mp_weight_srv[std::max(mip_levels - 9, 0)]);
		context.set_pipeline_resource("envmap_weight9", *mp_weight_srv[std::max(mip_levels - 10, 0)]);
	}

	//プリサンプル数を返す
	uint presample_count() const { return mp_sample_buf->num_elements(); }

private:

	shader_file_holder			m_shader_file;
	buffer_ptr					mp_sample_buf;
	shader_resource_view_ptr	mp_sample_srv;
	unordered_access_view_ptr	mp_sample_uav;
	texture_ptr					mp_weight_tex;
	shader_resource_view_ptr	mp_weight_srv[10];
	buffer_ptr					mp_unnormalized_cdf_buf;
	shader_resource_view_ptr	mp_unnormalized_cdf_srv;
	unordered_access_view_ptr	mp_unnormalized_cdf_uav;
	texture*					mp_cube_tex;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
