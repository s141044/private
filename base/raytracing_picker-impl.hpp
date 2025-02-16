
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//raytracing_picker
///////////////////////////////////////////////////////////////////////////////////////////////////

//�R���X�g���N�^
inline raytracing_picker::raytracing_picker(const uint max_size) : m_current(max_size, nullptr), mp_picked(), m_complete_frame(-1)
{
	m_shaders = gp_shader_manager->create(L"pick.sdf.json");
	mp_result_buf = gp_render_device->create_byteaddress_buffer(4, resource_flag_allow_unordered_access);
	mp_result_uav = gp_render_device->create_unordered_access_view(*mp_result_buf, buffer_uav_desc(*mp_result_buf));
	mp_readback_buf = gp_render_device->create_readback_buffer(4);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�C���X�^���X�o�^
inline void raytracing_picker::register_instance(render_entity &entity)
{
	m_current[entity.raytracing_instance_index()] = &entity;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�C���X�^���X����
inline void raytracing_picker::unregister_instance(render_entity &entity)
{
	m_current[entity.raytracing_instance_index()] = nullptr;

	//�s�b�N�v�����ɉ�����ꂽ���̂��L�^
	if(m_past.size()){ m_unregistered.push_back(&entity); }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�s�b�N�v��
inline void raytracing_picker::pick_request(render_context &context, const uint2 &pixel_pos)
{
	if(m_shaders.has_update())
		m_shaders.update();
	else if(m_shaders.is_invalid())
		return;

	push_priority push_priority(context);
	context.set_priority(priority_pick);
	context.set_pipeline_resource("AS", gp_raytracing_manager->tlas());
	context.set_pipeline_resource("raytracing_instance_descs", gp_raytracing_manager->raytracing_instance_descs_srv());
	context.set_pipeline_resource("result", *mp_result_uav);
	context.set_pipeline_state(*m_shaders.get("pick"));
	context.dispatch_with_32bit_constant(1, 1, 1, pixel_pos.x | (pixel_pos.y << 16));
	context.copy_buffer(*mp_readback_buf, 0, *mp_result_buf, 0, 4);

	m_past = m_current;
	m_complete_frame = gp_render_device->frame_count() + 3;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�s�b�N���ʂ�Ԃ�
inline render_entity *raytracing_picker::get()
{
	if(gp_render_device->frame_count() >= m_complete_frame)
	{
		m_complete_frame = -1;

		const auto index = mp_readback_buf->data<uint>()[0];
		if(index == 0xffffffff)
			mp_picked = nullptr;
		else
		{
			mp_picked = m_past[index];

			//�s�b�N���ɉ�����ꂽ�ꍇ�͖���
			if(std::find(m_unregistered.begin(), m_unregistered.end(), mp_picked) != m_unregistered.end()){ mp_picked = nullptr; }
		}
		m_unregistered.clear();
	}
	return mp_picked;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//�s�b�N���ʂ��N���A
inline void raytracing_picker::clear()
{
	mp_picked = nullptr;
	m_complete_frame = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
