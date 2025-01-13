
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_manager
///////////////////////////////////////////////////////////////////////////////////////////////////

//�R���X�g���N�^
inline texture_manager::texture_manager()
{
	//m_shaders = gp_shader_manager->create("mipmap.sdf.json");
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�쐬
inline shared_ptr<texture_manager::texture_resource> texture_manager::create(const string &filename, const texture_type type)
{
	//create_texture�n�ŏ���������Ƃ��̃f�[�^��LOD0�̕������Ȃ̂����ʂł���悤�ɂ��Ȃ��ƃ_���D���₯��

	auto result = m_texture_ptrs.emplace(format(filename) + std::to_string(type), nullptr);
	if(result.second)
	{
		texture_ptr p_tex;
		switch(type)
		{
		case texture_type_rgb:
		case texture_type_srgb:
		{
			texture_format format;
			if(type == texture_type_rgb)
				format = texture_format_r8g8b8a8_unorm;
			else if(type == texture_type_srgb)
				format = texture_format_r8g8b8a8_unorm_srgb;

			image src(filename);
			p_tex = gp_render_device->create_texture2d(format, src.width(), src.height(), 1, resource_flag_allow_shader_resource, src(0, 0));
			break;
		}
		case texture_type_normal:
		{
			normal_map src(filename);
			p_tex = gp_render_device->create_texture2d(texture_format_r8g8b8a8_unorm, src.width(), src.height(), 1, resource_flag_allow_shader_resource, src(0, 0));
			break;
		}
		case texture_type_scalar:
		{
			image src(filename);
			vector<unsigned char> red(src.width() * src.height());
			for(int y = 0, i = 0; y < src.height(); y++)
				for(int x = 0; x < src.width(); x++)
					red[i++] = src(x, y)[0];
			p_tex = gp_render_device->create_texture2d(texture_format_r8_unorm, src.width(), src.height(), 1, resource_flag_allow_shader_resource, red.data());
			break;
		}}

		auto &p_res = result.first->second;
		p_res.reset(new texture_resource(std::move(p_tex), result.first));
		p_res->mp_srv = gp_render_device->create_shader_resource_view(*p_res->mp_tex, texture_srv_desc(*p_res->mp_tex));
		gp_render_device->register_bindless(*p_res->mp_srv);
		m_queue.push(p_res);
	}
	return result.first->second;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�X�V
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
	
		//�~�b�v�}�b�v����
		//

		p_res->m_initialized = true;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//texture_manager::texture_resource
///////////////////////////////////////////////////////////////////////////////////////////////////

//�R���X�g���N�^
inline texture_manager::texture_resource::texture_resource(texture_ptr p_tex, dictionary::iterator it) : mp_tex(std::move(p_tex)), m_it(it)
{
	m_initialized = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�f�X�g���N�^
inline texture_manager::texture_resource::~texture_resource()
{
	if(mp_srv && mp_srv->bindless_handle() != invalid_bindless_handle)
		gp_render_device->unregister_bindless(*mp_srv);

	gp_texture_manager->m_texture_ptrs.erase(m_it);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//SRV��Ԃ�
inline shader_resource_view &texture_manager::texture_resource::srv() const
{
	return *mp_srv;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
