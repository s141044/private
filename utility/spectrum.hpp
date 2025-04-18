
#ifndef NN_RENDER_UTILITY_SPECTRUM_HPP
#define NN_RENDER_UTILITY_SPECTRUM_HPP

#include"../base.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//spectral_data
///////////////////////////////////////////////////////////////////////////////////////////////////

class spectral_data
{
public:

	//バインド
	void bind(render_context& context)
	{
		if(mp_tex == nullptr)
		{
#define CIE_LAMBDA_MIN 360.0
#define CIE_LAMBDA_MAX 830.0
#define CIE_SAMPLES 95

			const float cie_x[CIE_SAMPLES] = {
				0.000129900000f, 0.000232100000f, 0.000414900000f, 0.000741600000f, 0.001368000000f,
				0.002236000000f, 0.004243000000f, 0.007650000000f, 0.014310000000f, 0.023190000000f,
				0.043510000000f, 0.077630000000f, 0.134380000000f, 0.214770000000f, 0.283900000000f,
				0.328500000000f, 0.348280000000f, 0.348060000000f, 0.336200000000f, 0.318700000000f,
				0.290800000000f, 0.251100000000f, 0.195360000000f, 0.142100000000f, 0.095640000000f,
				0.057950010000f, 0.032010000000f, 0.014700000000f, 0.004900000000f, 0.002400000000f,
				0.009300000000f, 0.029100000000f, 0.063270000000f, 0.109600000000f, 0.165500000000f,
				0.225749900000f, 0.290400000000f, 0.359700000000f, 0.433449900000f, 0.512050100000f,
				0.594500000000f, 0.678400000000f, 0.762100000000f, 0.842500000000f, 0.916300000000f,
				0.978600000000f, 1.026300000000f, 1.056700000000f, 1.062200000000f, 1.045600000000f,
				1.002600000000f, 0.938400000000f, 0.854449900000f, 0.751400000000f, 0.642400000000f,
				0.541900000000f, 0.447900000000f, 0.360800000000f, 0.283500000000f, 0.218700000000f,
				0.164900000000f, 0.121200000000f, 0.087400000000f, 0.063600000000f, 0.046770000000f,
				0.032900000000f, 0.022700000000f, 0.015840000000f, 0.011359160000f, 0.008110916000f,
				0.005790346000f, 0.004109457000f, 0.002899327000f, 0.002049190000f, 0.001439971000f,
				0.000999949300f, 0.000690078600f, 0.000476021300f, 0.000332301100f, 0.000234826100f,
				0.000166150500f, 0.000117413000f, 0.000083075270f, 0.000058706520f, 0.000041509940f,
				0.000029353260f, 0.000020673830f, 0.000014559770f, 0.000010253980f, 0.000007221456f,
				0.000005085868f, 0.000003581652f, 0.000002522525f, 0.000001776509f, 0.000001251141f};

			const float cie_y[CIE_SAMPLES] = {
				0.000003917000f, 0.000006965000f, 0.000012390000f, 0.000022020000f, 0.000039000000f,
				0.000064000000f, 0.000120000000f, 0.000217000000f, 0.000396000000f, 0.000640000000f,
				0.001210000000f, 0.002180000000f, 0.004000000000f, 0.007300000000f, 0.011600000000f,
				0.016840000000f, 0.023000000000f, 0.029800000000f, 0.038000000000f, 0.048000000000f,
				0.060000000000f, 0.073900000000f, 0.090980000000f, 0.112600000000f, 0.139020000000f,
				0.169300000000f, 0.208020000000f, 0.258600000000f, 0.323000000000f, 0.407300000000f,
				0.503000000000f, 0.608200000000f, 0.710000000000f, 0.793200000000f, 0.862000000000f,
				0.914850100000f, 0.954000000000f, 0.980300000000f, 0.994950100000f, 1.000000000000f,
				0.995000000000f, 0.978600000000f, 0.952000000000f, 0.915400000000f, 0.870000000000f,
				0.816300000000f, 0.757000000000f, 0.694900000000f, 0.631000000000f, 0.566800000000f,
				0.503000000000f, 0.441200000000f, 0.381000000000f, 0.321000000000f, 0.265000000000f,
				0.217000000000f, 0.175000000000f, 0.138200000000f, 0.107000000000f, 0.081600000000f,
				0.061000000000f, 0.044580000000f, 0.032000000000f, 0.023200000000f, 0.017000000000f,
				0.011920000000f, 0.008210000000f, 0.005723000000f, 0.004102000000f, 0.002929000000f,
				0.002091000000f, 0.001484000000f, 0.001047000000f, 0.000740000000f, 0.000520000000f,
				0.000361100000f, 0.000249200000f, 0.000171900000f, 0.000120000000f, 0.000084800000f,
				0.000060000000f, 0.000042400000f, 0.000030000000f, 0.000021200000f, 0.000014990000f,
				0.000010600000f, 0.000007465700f, 0.000005257800f, 0.000003702900f, 0.000002607800f,
				0.000001836600f, 0.000001293400f, 0.000000910930f, 0.000000641530f, 0.000000451810f};

			const float cie_z[CIE_SAMPLES] = {
				0.000606100000f, 0.001086000000f, 0.001946000000f, 0.003486000000f, 0.006450001000f,
				0.010549990000f, 0.020050010000f, 0.036210000000f, 0.067850010000f, 0.110200000000f,
				0.207400000000f, 0.371300000000f, 0.645600000000f, 1.039050100000f, 1.385600000000f,
				1.622960000000f, 1.747060000000f, 1.782600000000f, 1.772110000000f, 1.744100000000f,
				1.669200000000f, 1.528100000000f, 1.287640000000f, 1.041900000000f, 0.812950100000f,
				0.616200000000f, 0.465180000000f, 0.353300000000f, 0.272000000000f, 0.212300000000f,
				0.158200000000f, 0.111700000000f, 0.078249990000f, 0.057250010000f, 0.042160000000f,
				0.029840000000f, 0.020300000000f, 0.013400000000f, 0.008749999000f, 0.005749999000f,
				0.003900000000f, 0.002749999000f, 0.002100000000f, 0.001800000000f, 0.001650001000f,
				0.001400000000f, 0.001100000000f, 0.001000000000f, 0.000800000000f, 0.000600000000f,
				0.000340000000f, 0.000240000000f, 0.000190000000f, 0.000100000000f, 0.000049999990f,
				0.000030000000f, 0.000020000000f, 0.000010000000f, 0.000000000000f, 0.000000000000f,
				0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
				0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
				0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
				0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
				0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
				0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f,
				0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f, 0.000000000000f};

			double3 sum(0.0);
			vector<float4> xyz(CIE_SAMPLES);
			for(size_t i = 0; i < xyz.size(); i++)
			{
				xyz[i] = float4(cie_x[i], cie_y[i], cie_z[i], 0);
				sum[0] += cie_x[i];
				sum[1] += cie_y[i];
				sum[2] += cie_z[i];
			}

			sum *= (CIE_LAMBDA_MAX - CIE_LAMBDA_MIN) / float(CIE_SAMPLES - 1);

			const auto scale = float3(
				float(1.0 / sum[0]),
				float(1.0 / sum[1]),
				float(1.0 / sum[2]));

			for(size_t i = 0; i < xyz.size(); i++)
				xyz[i].xyz *= scale;

			//フォーマット嫌だけど...
			mp_tex = gp_render_device->create_texture1d(texture_format_r32g32b32a32_float, CIE_SAMPLES, 1, resource_flag_allow_shader_resource, xyz.data());
			mp_srv = gp_render_device->create_shader_resource_view(*mp_tex, texture_srv_desc(*mp_tex));
		}
		context.set_pipeline_resource("cie_xyz_srv", *mp_srv);
	}

private:

	texture_ptr					mp_tex;
	shader_resource_view_ptr	mp_srv;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
//wavelength_sampler
///////////////////////////////////////////////////////////////////////////////////////////////////

class wavelength_sampler
{
public:

	//コンストラクタ
	wavelength_sampler() : m_engine(std::random_device{}())
	{
		m_shaders = gp_shader_manager->create(L"wavelength.sdf.json");
	}

	//サンプリング
	bool sample(render_context& context, const uint sample_count)
	{
		if(m_shaders.has_update())
			m_shaders.update();
		else if(m_shaders.is_invalid())
			return false;

		struct sample
		{
			uint lambda	: 16;
			uint pdf	: 16;
		};

		if((mp_sample_buf == nullptr) || (mp_sample_buf->num_elements() != sample_count))
		{
			mp_sample_buf = gp_render_device->create_structured_buffer(sizeof(sample), sample_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_sample_srv = gp_render_device->create_shader_resource_view(*mp_sample_buf, buffer_srv_desc(*mp_sample_buf));
			mp_sample_uav = gp_render_device->create_unordered_access_view(*mp_sample_buf, buffer_uav_desc(*mp_sample_buf));
		}

		struct cbuffer
		{
			uint	sample_count;
			float2	jitter;
			float	padding;
		};

		auto p_cbuf = gp_render_device->create_temporary_cbuffer(sizeof(cbuffer));
		auto& cbuf_data = *p_cbuf->data<cbuffer>();
		cbuf_data.sample_count = sample_count;
		cbuf_data.jitter.x = std::uniform_real_distribution<float>{}(m_engine);
		cbuf_data.jitter.y = std::uniform_real_distribution<float>{}(m_engine);

		context.set_pipeline_resource("wavelength_sample_uav", *mp_sample_uav);
		context.set_pipeline_resource("presample_wavelength_cbuffer", *p_cbuf);
		context.set_pipeline_state(*m_shaders.get("presample_wavelength"));
		context.dispatch(ceil_div(ceil_div(sample_count ,2), 256), 1, 1);
		return true;
	}

	//バインド
	void bind(render_context& context)
	{
		context.set_pipeline_resource("wavelength_sample_srv", *mp_sample_srv);
	}

private:

	shader_file_holder			m_shaders;
	buffer_ptr					mp_sample_buf;
	shader_resource_view_ptr	mp_sample_srv;
	unordered_access_view_ptr	mp_sample_uav;
	std::mt19937_64				m_engine;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
