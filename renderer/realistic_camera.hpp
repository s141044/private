
#pragma once

#ifndef NN_RENDER_RENDERER_REALISTIC_CAMERA_HPP
#define NN_RENDER_RENDERER_REALISTIC_CAMERA_HPP

#include"../base.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//realistic_camera
///////////////////////////////////////////////////////////////////////////////////////////////////

class realistic_camera
{
public:

	//コンストラクタ
	realistic_camera()
	{
		m_shader_file = gp_shader_manager->create(L"renderer/realistic_camera.sdf.json");

		//std::ifstream ifs("lens/wide.22mm.dat");
		std::ifstream ifs("lens/dgauss.dat");
		if(!ifs.is_open())
			throw;
		
		string line;
		while(not(ifs.eof()))
		{
			std::getline(ifs, line);
			if(line.empty() || line[0] == '#')
				continue;

			std::stringstream ss(line);
			auto& iface = m_interfaces.emplace_back();
			ss >> iface.curvature_radius;
			ss >> iface.thickness;
			ss >> iface.ior[0];
			ss >> iface.aperture_radius;
			iface.aperture_radius /= 2;

			iface.thickness /= 1000;
			iface.aperture_radius /= 1000;
			iface.curvature_radius /= 1000;
		}

		//厚みパラメータはセンサ側が0になるように調整
		if(m_interfaces.front().thickness == 0)
		{
			for(size_t i = 1; i < m_interfaces.size(); i++)
				m_interfaces[i - 1].thickness = m_interfaces[i].thickness;
			m_interfaces.back().thickness = 0;
		}

		//センサ側が先頭に来るように並び替え
		std::reverse(m_interfaces.begin(), m_interfaces.end());
		
		//絶対距離に変更
		for(size_t i = 1; i < m_interfaces.size(); i++)
		{
			m_interfaces[i].thickness = -m_interfaces[i].thickness;
			m_interfaces[i].thickness += m_interfaces[i - 1].thickness;
		}

		//センサ側からのトレースでiorを扱いやすいように
		for(size_t i = 0; i + 1 < m_interfaces.size(); i++)
		{
			if(m_interfaces[i + 1].ior[0] > 0)
				m_interfaces[i].ior[1] = m_interfaces[i + 1].ior[0];
			else if(i + 2 < m_interfaces.size())
				m_interfaces[i].ior[1] = m_interfaces[i + 2].ior[0];
			else
				m_interfaces[i].ior[1] = 1;
		}
		m_interfaces.back().ior[1] = 1;

		//前側焦点/主点を計算
		ray sensor_in, scene_out;
		for(uint i = 1; i < 10; i++)
		{
			sensor_in.o = float3(m_interfaces.front().aperture_radius / (1 << i), 0, m_interfaces.front().thickness + 1);
			sensor_in.d = float3(0, 0, -1);
			if(trace(sensor_in, scene_out))
				break;
		}
		const float front_tf = -scene_out.o.x / scene_out.d.x;
		const float front_tp = (sensor_in.o.x - scene_out.o.x) / scene_out.d.x;
		const float front_fz = (scene_out.o.z + scene_out.d.z * front_tf);
		const float front_pz = (scene_out.o.z + scene_out.d.z * front_tp);

		//後側焦点/主点を計算
		ray scene_in, sensor_out;
		for(uint i = 1; i < 10; i++)
		{
			scene_in.o = float3(-m_interfaces.back().aperture_radius / (1 << i), 0, m_interfaces.back().thickness - 1);
			scene_in.d = float3(0, 0, 1);
			if(trace(scene_in, sensor_out, false))
				break;
		}
		const float back_tf = -sensor_out.o.x / sensor_out.d.x;
		const float back_tp = (scene_in.o.x - sensor_out.o.x) / sensor_out.d.x;
		const float back_fz = (sensor_out.o.z + sensor_out.d.z * back_tf);
		const float back_pz = (sensor_out.o.z + sensor_out.d.z * back_tp);

		//後側主点がz=0になるように調整
		for(size_t i = 0; i < m_interfaces.size(); i++)
			m_interfaces[i].thickness -= back_pz;
		m_front_fz = front_fz - back_pz;
		m_front_pz = front_pz - back_pz;
		m_back_fz = back_fz - back_pz;
		m_back_pz = 0;

		struct gpu_interface_t
		{
			float aperture_radius;
			float curvature_radius;
			float thickness;
			float ior;
		};
		vector<gpu_interface_t> gpu_interfaces(m_interfaces.size());
		for(size_t i = 0; i < m_interfaces.size(); i++)
		{
			gpu_interfaces[i].aperture_radius = m_interfaces[i].aperture_radius;
			gpu_interfaces[i].curvature_radius = m_interfaces[i].curvature_radius;
			gpu_interfaces[i].thickness = m_interfaces[i].thickness;
			gpu_interfaces[i].ior = m_interfaces[i].ior[1];
		}
		mp_interface_buf = gp_render_device->create_structured_buffer(sizeof(gpu_interface_t), uint(m_interfaces.size()), resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access), gpu_interfaces.data());
		mp_interface_srv = gp_render_device->create_shader_resource_view(*mp_interface_buf, buffer_srv_desc(*mp_interface_buf));
		mp_interface_uav = gp_render_device->create_unordered_access_view(*mp_interface_buf, buffer_uav_desc(*mp_interface_buf));
		gp_render_device->set_name(*mp_interface_buf, L"interface_buf");

		mp_pupil_buf = gp_render_device->create_byteaddress_buffer(sizeof(uint4) * m_r_divide_count, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
		mp_pupil_srv = gp_render_device->create_shader_resource_view(*mp_pupil_buf, buffer_srv_desc(*mp_pupil_buf));
		mp_pupil_uav = gp_render_device->create_unordered_access_view(*mp_pupil_buf, buffer_uav_desc(*mp_pupil_buf));
		gp_render_device->set_name(*mp_pupil_buf, L"pupil_buf");
	}

	//実行
	bool initialize(render_context &context, const camera& camera)
	{
		if(m_shader_file.has_update())
			m_shader_file.update();
		else if(m_shader_file.is_invalid())
			return false;

		if((m_fovy != camera.fovy()) || (m_distance != camera.distance()))
		{
			const float f = m_front_pz - m_front_fz;
			const float a = m_front_pz + camera.distance();
			const float b = a * f / (a - f);
			m_delta = b - (m_interfaces.front().thickness - m_back_pz);

			if((f <= 0) || (a <= 0) || (b <= 0) || (m_delta <= 0))
				return false;

			m_fovy = camera.fovy();
			m_distance = camera.distance();

			const float height = b * tan(to_radian(camera.fovy()) / 2) * 2;
			const float width = height * camera.aspect();
			const float diag = sqrt(width * width + height * height);

			struct cbuffer
			{
				float	delta_z;
				float	delta_r;
				float	inv_delta_r;
				uint	interface_count;
				float2	sensor_min;
				float2	sensor_size;
			};
			mp_cbuf = gp_render_device->create_temporary_cbuffer(sizeof(cbuffer));
			auto& cbuf_data = *mp_cbuf->data<cbuffer>();
			cbuf_data.delta_z = m_delta;
			cbuf_data.delta_r = diag / (2 * m_r_divide_count);
			cbuf_data.inv_delta_r = (2 * m_r_divide_count) / diag;
			cbuf_data.interface_count = uint(m_interfaces.size());
			cbuf_data.sensor_min = -float2(width, height) / 2;
			cbuf_data.sensor_size = float2(width, height);

			context.set_pipeline_resource("realistic_camera_cb", *mp_cbuf);
			context.set_pipeline_resource("realistic_camera_pupil_uav", *mp_pupil_uav);
			context.set_pipeline_resource("realistic_camera_interface_srv", *mp_interface_srv);
			context.set_pipeline_state(*m_shader_file.get("init_pupil"));
			context.dispatch(ceil_div(m_r_divide_count, 256), 1, 1);
			context.set_pipeline_state(*m_shader_file.get("calc_pupil"));
			context.dispatch_with_32bit_constant(ceil_div(m_sample_count, 256), m_r_divide_count, 1, m_sample_count);
		}
		
		if(mp_aperture_tex == nullptr)
		{
			const uint w = 512;
			const uint h = 512;
			mp_aperture_tex = gp_render_device->create_texture2d(texture_format_r8_unorm, w, h, 1, resource_flags(resource_flag_allow_shader_resource | resource_flag_allow_unordered_access));
			mp_aperture_srv = gp_render_device->create_shader_resource_view(*mp_aperture_tex, texture_srv_desc(*mp_aperture_tex));
			mp_aperture_uav = gp_render_device->create_unordered_access_view(*mp_aperture_tex, texture_uav_desc(*mp_aperture_tex));

			struct cbuffer
			{
				float	angle;
				float	cos_angle_x05;
				float2	inv_size;
			};
			auto p_cbuf = gp_render_device->create_temporary_cbuffer(sizeof(cbuffer));
			auto& cbuf_data = *p_cbuf->data<cbuffer>();
			const float angle = 2 * PI() / m_blade_count;
			cbuf_data.angle = angle;
			cbuf_data.cos_angle_x05 = cos(angle / 2);
			cbuf_data.inv_size.x = 1 / float(w);
			cbuf_data.inv_size.y = 1 / float(h);

			context.set_pipeline_resource("realistic_camera_aperture_cb", *p_cbuf);
			context.set_pipeline_resource("realistic_camera_aperture_uav", *mp_aperture_uav);
			context.set_pipeline_state(*m_shader_file.get("calc_aperture"));
			context.dispatch(ceil_div(w, 16), ceil_div(h, 16), 1);
		}

//#define REALISTIC_CAMERA_DEBUG_DRAW
#if defined(REALISTIC_CAMERA_DEBUG_DRAW)
		for(auto& iface : m_interfaces)
		{
			const float r = abs(iface.curvature_radius);
			if(r == 0)
			{
				auto* vertices = gp_debug_draw->draw_lines(4, true);

				const float cz = iface.thickness;
				vertices[0] = debug_draw::vertex(float3(+1, 0, cz), float3(1));
				vertices[2] = debug_draw::vertex(float3(-1, 0, cz), float3(1));
				vertices[1] = debug_draw::vertex(float3(+iface.aperture_radius, 0, cz), float3(1));
				vertices[3] = debug_draw::vertex(float3(-iface.aperture_radius, 0, cz), float3(1));
			}
			else
			{
				const uint N = 100;
				auto* vertices = gp_debug_draw->draw_line_strip(N - 1, true);

				const float cz = iface.thickness + iface.curvature_radius;
				const float th_max = asin(iface.aperture_radius / r);
				const float dth = 2 * th_max / (N - 1);
				for(uint i = 0; i < N; i++)
				{
					const float th = th_max - dth * i;
					const float st = sin(th);
					const float ct = cos(th);

					auto p = float3(r * st, 0, cz);
					if(iface.curvature_radius < 0)
						p.z += r * ct;
					else
						p.z -= r * ct;

					vertices[i] = debug_draw::vertex(p, float3(1));
				}
			}
		}
		
		auto* v = gp_debug_draw->draw_lines(2, true);
		v[0] = debug_draw::vertex(float3(+1, 0, -m_distance), float3(1, 0, 0));
		v[1] = debug_draw::vertex(float3(-1, 0, -m_distance), float3(1, 0, 0));
		v[2] = debug_draw::vertex(float3(0, 0, +10), float3(1, 1, 1));
		v[3] = debug_draw::vertex(float3(0, 0, -10), float3(1, 1, 1));

		std::mt19937_64 engine;
		for(uint i = 0, j = 0; i < 1024; i++)
		{
			float2 u;
			u[0] = std::uniform_real_distribution<float>{}(engine);
			u[1] = std::uniform_real_distribution<float>{}(engine);
				
			float ar = m_interfaces[0].aperture_radius;
			float3 target = float3(sample_uniform_disk(ar, u[0], u[1]), m_interfaces.front().thickness);
				
			float3 o = float3(0, 0, m_interfaces.front().thickness + m_delta);
			float3 d = normalize(target - o);
				
			ray r, out;
			r.o = o;
			r.d = d;
			j += trace(r, out);
			if(j >= 128)
				break;
		}
#endif
		return true;
	}

	//リソースをバインド
	void bind(render_context& context)
	{
		context.set_pipeline_resource("realistic_camera_cb", *mp_cbuf);
		context.set_pipeline_resource("realistic_camera_pupil_srv", *mp_pupil_srv);
		context.set_pipeline_resource("realistic_camera_aperture_srv", *mp_aperture_srv);
		context.set_pipeline_resource("realistic_camera_interface_srv", *mp_interface_srv);
	}

	//パラメータ
	uint blade_count() const { return m_blade_count; }
	void set_blade_count(const uint n){ if(m_blade_count != n){ m_blade_count = n; mp_aperture_tex.reset(); }}

private:

	float2 sample_uniform_disk(float R, float u0, float u1)
	{
		u0 = 2 * u0 - 1;
		u1 = 2 * u1 - 1;

		if(u0 == 0 && u1 == 0)
			return 0;

		float r, theta;
		if(abs(u0) > abs(u1))
		{
			r = R * u0;
			theta = (PI() / 4) * (u1 / u0);
		}
		else
		{
			r = R * u1;
			theta = (PI() / 2) - (PI() / 4) * (u0 / u1);
		}
		float2 ret = r * float2(cos(theta), sin(theta));
		return r * float2(cos(theta), sin(theta));
	}

	//交差判定
	bool intersect(const float cz, const float r, const float ar, const float3& o, const float3& d, float& t)
	{
		const float3 co = float3(o.x, o.y, o.z - cz);
		const float B = dot(d, co);
		const float C = dot(co, co) - r * r;
		const float D = B * B - C;
		if(D <= 0)
			return false;

		const float sqrt_D = sqrt(D);
		const float t0 = -B - sqrt_D;
		const float t1 = -B + sqrt_D;
		t = (t0 > 0) ? t0 : t1;

		const float2 p = o.xy + t * d.xy;
		return (dot(p, p) < ar * ar);
	}
	bool intersect(const float cz, const float ar, const float3& o, const float3& d, float& t)
	{
		t = (cz - o.z) / d.z;

		const float2 p = o.xy + t * d.xy;
		return (dot(p, p) >= ar * ar);
	}

	//屈折方向を計算
	float3 refract(const float3& i, const float3& n, const float eta)
	{
		const float cos = dot(i, n);
		const float k = 1 - eta * eta * (1 - cos * cos);
		if(k <= 0)
			return float3(0, 0, 0);
		else
			return eta * i - (eta * cos + sqrt(k)) * n;
	}

	//レイトレース
	struct ray{ float3 o, d; };
	bool trace(const ray& in, ray& out, const bool forward = true)
	{
		int i = 0;
		int n = int(m_interfaces.size());
		int step = 1;

		if(not(forward))
		{
			i = int(m_interfaces.size()) - 1;
			n = -1;
			step = -1;
		}

		out = in;

#if defined(REALISTIC_CAMERA_DEBUG_DRAW)
		vector<float3> vertices;
#endif
		float ior = 1;
		for(; i != n; i += step)
		{
			const auto& iface = m_interfaces[i];
			const float cz = iface.thickness + iface.curvature_radius;

			//絞り
			if(iface.curvature_radius == 0)
			{
				float t;
				if(intersect(cz, iface.aperture_radius, out.o, out.d, t))
					return false;

#if defined(REALISTIC_CAMERA_DEBUG_DRAW)
				vertices.push_back(out.o);
#endif
				out.o += out.d * t;
			}
			//レンズ
			else
			{
				float t;
				if(not(intersect(cz, iface.curvature_radius, iface.aperture_radius, out.o, out.d, t)))
					return false;

#if defined(REALISTIC_CAMERA_DEBUG_DRAW)
				vertices.push_back(out.o);
#endif
				out.o += out.d * t;

				float3 normal = normalize(float3(out.o.xy, out.o.z - cz));
				if(dot(normal, out.d) > 0)
					normal = -normal;

				out.d = refract(out.d, normal, ior / iface.ior[forward]);
				ior = iface.ior[forward];
			}
		}
#if defined(REALISTIC_CAMERA_DEBUG_DRAW)

		auto* v = gp_debug_draw->draw_line_strip(uint(vertices.size()) + 1, true);

		float3 col;
		col[0] = float(forward);
		col[1] = float(1 - forward);
		col[2] = float(1);

		size_t j = 0;
		for(0; j < vertices.size(); j++)
			v[j] = debug_draw::vertex(vertices[j], col);
		v[j++] = debug_draw::vertex(out.o, col);
		v[j++] = debug_draw::vertex(out.o + out.d * 100, col);

#endif
		return true;
	}

private:

	static constexpr uint		m_sample_count = 1024 * 1024;
	static constexpr uint		m_r_divide_count = 64;

	shader_file_holder			m_shader_file;
	constant_buffer_ptr			mp_cbuf;
	buffer_ptr					mp_pupil_buf;
	shader_resource_view_ptr	mp_pupil_srv;
	unordered_access_view_ptr	mp_pupil_uav;
	buffer_ptr					mp_interface_buf;
	shader_resource_view_ptr	mp_interface_srv;
	unordered_access_view_ptr	mp_interface_uav;
	texture_ptr					mp_aperture_tex;
	shader_resource_view_ptr	mp_aperture_srv;
	unordered_access_view_ptr	mp_aperture_uav;

	struct interface_t
	{
		float aperture_radius;
		float curvature_radius;
		float thickness;
		float ior[2];
	};
	vector<interface_t>		m_interfaces;
	float					m_distance = -1;
	float					m_delta = -1;
	float					m_fovy = -1;
	float					m_front_fz;
	float					m_front_pz;
	float					m_back_fz;
	float					m_back_pz;
	uint					m_blade_count = 6;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif
