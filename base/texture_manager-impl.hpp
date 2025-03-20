
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_manager
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline texture_manager::texture_manager()
{
	//m_shaders = gp_shader_manager->create("mipmap.sdf.json");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//デストラクタ
inline texture_manager::~texture_manager()
{
	for(auto& [key, p_tex] : m_texture_ptrs)
	{
		if(p_tex.use_count() > 1)
		{
			std::cout << p_tex->m_filename << (char*)u8"が解放されていません";
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//作成
inline shared_ptr<texture_manager::texture_resource> texture_manager::create(const string &filename, const texture_type type)
{
	//TODO: ミップマップ対応
	//レイトレなのでテクスチャを使う側でもミップレベル指定の対応をしないとダメ

	auto mip_levels = [](const uint width, const uint height)
	{
		uint size = std::max(width, height);
		for(uint mip_levels = 1; ; mip_levels++)
		{
			if((size >> mip_levels) == 0)
				return mip_levels;
		}
	};
	auto to_rgba = [](const image& src, const unsigned char alpha)
	{
		image dst(src.width(), src.height(), 4);
		for(int y = 0; y < src.height(); y++)
		{
			for(int x = 0; x < src.width(); x++)
			{
				dst(x, y)[0] = src(x, y)[0];
				dst(x, y)[1] = src(x, y)[1];
				dst(x, y)[2] = src(x, y)[2];
				dst(x, y)[3] = alpha;
			}
		}
		return dst;
	};
	auto r8g8b8e8_to_f32x3 = [](const unsigned char rgbe[4])
	{
		const unsigned char r = rgbe[0];
		const unsigned char g = rgbe[1];
		const unsigned char b = rgbe[2];
		const unsigned char e = rgbe[3];
		if(e == 0)
			return float3(0, 0, 0);
		
		const auto scale = scalbn(1.0f, e - (128 + 8));
		return float3(r, g, b) * scale;
	};
	auto r8g8b8e8_to_r9g9b9e5 = [&](const unsigned char rgbe[4])
	{
		return f32x3_to_r9g9b9e5(r8g8b8e8_to_f32x3(rgbe));
	};

	auto result = m_texture_ptrs.emplace(format(filename) + "_" + std::to_string(type), nullptr);
	if(result.second)
	{
		texture_ptr p_tex;
		switch(type)
		{
		case texture_type_hdr:
		{
			hdr_image src(filename);
			Image<unsigned int> r9g9b9e5(src.width(), src.height(), 1);
			for(int y = 0; y < src.height(); y++)
				for(int x = 0; x < src.width(); x++)
					r9g9b9e5(x, y)[0] = r8g8b8e8_to_r9g9b9e5(src(x, src.height() - 1 - y));
			p_tex = gp_render_device->create_texture2d(texture_format_r9g9b9e5_sharedexp, src.width(), src.height(), 1, resource_flag_allow_shader_resource, r9g9b9e5(0, 0), true);
			break;
		}
		case texture_type_rgb:
		case texture_type_srgb:
		{
			texture_format format;
			if(type == texture_type_rgb)
				format = texture_format_r8g8b8a8_unorm;
			else if(type == texture_type_srgb)
				format = texture_format_r8g8b8a8_unorm_srgb;

			image src(filename);
			if(src.channels() != 4){ src = to_rgba(src, 255); }
			p_tex = gp_render_device->create_texture2d(format, src.width(), src.height(), 1, resource_flag_allow_shader_resource, src(0, 0), true);
			break;
		}
		case texture_type_normal:
		{
			image src = normal_map(filename);
			if(src.channels() != 4){ src = to_rgba(src, 255); }
			p_tex = gp_render_device->create_texture2d(texture_format_r8g8b8a8_unorm, src.width(), src.height(), 1, resource_flag_allow_shader_resource, src(0, 0), true);
			break;
		}
		case texture_type_scalar:
		{
			image src(filename);
			image red(src.width(), src.height(), 1);
			for(int y = 0; y < src.height(); y++)
				for(int x = 0; x < src.width(); x++)
					red(x, y)[0] = src(x, y)[0];
			p_tex = gp_render_device->create_texture2d(texture_format_r8_unorm, src.width(), src.height(), 1, resource_flag_allow_shader_resource, red(0, 0), true);
			break;
		}}

		auto &p_res = result.first->second;
		p_res.reset(new texture_resource(std::move(p_tex), filename, result.first));
		p_res->mp_srv = gp_render_device->create_shader_resource_view(*p_res->mp_tex, texture_srv_desc(*p_res->mp_tex));
		gp_render_device->register_bindless(*p_res->mp_srv);
		m_queue.push(p_res);
	}
	return result.first->second;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//更新
inline bool texture_manager::update(render_context &context)
{
	//if(m_shaders.has_update())
	//	m_shaders.update();
	//else if(m_shaders.is_invalid())
	//	return false;
	
	while(m_queue.size())
	{
		auto p_res = m_queue.front().lock();
		m_queue.pop();
	
		if((p_res == nullptr) || p_res->m_initialized)
			continue;
	
		//ミップマップ生成
		//

		p_res->m_initialized = true;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_manager::texture_resource
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline texture_manager::texture_resource::texture_resource(texture_ptr p_tex, string filename, dictionary::iterator it) : mp_tex(std::move(p_tex)), m_filename(std::move(filename)), m_it(it)
{
	m_initialized = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//デストラクタ
inline texture_manager::texture_resource::~texture_resource()
{
	if(mp_srv && mp_srv->bindless_handle() != invalid_bindless_handle)
		gp_render_device->unregister_bindless(*mp_srv);

	if(gp_texture_manager)
		gp_texture_manager->m_texture_ptrs.erase(m_it);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//SRVを返す
inline shader_resource_view& texture_manager::texture_resource::srv() const
{
	return *mp_srv;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ファイル名を返す
inline const string& texture_manager::texture_resource::filename() const
{
	return m_filename;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
