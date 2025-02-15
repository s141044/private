
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//base_application
///////////////////////////////////////////////////////////////////////////////////////////////////

//初期化
inline void base_application::initialize()
{
	m_present_shaders = gp_shader_manager->create(L"prepare_present.sdf.json");

	const uint max_instance_count = 65535;
	gp_texture_manager.reset(new texture_manager());
	gp_raytracing_picker.reset(new raytracing_picker(max_instance_count));
	gp_raytracing_manager.reset(new raytracing_manager(max_instance_count));

	m_camera.look_at(float3(0, 1, 10), float3(0, 0, 0));
	m_camera.set_fovy(60);
	m_camera.set_near_clip(0.01f);
	m_camera.set_far_clip(100.0f);

	initialize_impl();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//終了処理
inline void base_application::finalize()
{
	finalize_impl();
		
	mp_ldr_target.reset();
	mp_ldr_color_tex.reset();
	mp_ldr_color_rtv.reset();
	mp_ldr_color_srv.reset();
	mp_dummy_geometry.reset();
	for(uint i = 0; i < swapchain::buffer_count(); i++)
		mp_present_target[i].reset();

	gp_raytracing_manager.reset();
	gp_raytracing_picker.reset();
	gp_texture_manager.reset();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//更新
inline int base_application::update(render_context& context, const float delta_time)
{
	gp_texture_manager->update(context);
	return update_impl(context, delta_time);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//リサイズ
inline void base_application::resize(const uint2 size)
{
	mp_dummy_geometry = gp_render_device->create_geometry_state(0, nullptr, nullptr, nullptr, nullptr);
	mp_ldr_color_tex = gp_render_device->create_texture2d(ldr_texture_format(), size.x, size.y, 1, resource_flags(resource_flag_allow_render_target | resource_flag_allow_shader_resource | resource_flag_scratch));
	mp_ldr_color_rtv = gp_render_device->create_render_target_view(*mp_ldr_color_tex, texture_rtv_desc(*mp_ldr_color_tex));
	mp_ldr_color_srv = gp_render_device->create_shader_resource_view(*mp_ldr_color_tex, texture_srv_desc(*mp_ldr_color_tex));
	gp_render_device->set_name(*mp_ldr_color_tex, L"ldr_color_tex");

	render_target_view* rtv_ptrs[] = { mp_ldr_color_rtv.get(), };
	mp_ldr_target = gp_render_device->create_target_state(1, rtv_ptrs, nullptr);

	for(uint i = 0; i < swapchain::buffer_count(); i++)
	{
		render_target_view* rtv_ptrs[] = { &back_buffer_rtv(i) };
		mp_present_target[i] = gp_render_device->create_target_state(1, rtv_ptrs, nullptr);
	}

	m_camera.set_aspect(size.x / float(size.y));

	resize_impl(size);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//コンスタントバッファの更新
inline void base_application::update_scene_info(render_context& context, const uint2 screen_size, const float jitter_scale)
{
	struct scene_info_t
	{
		uint2		screen_size;
		float2		inv_screen_size;
		float3		camera_pos;
		float		pixel_size;		//距離1におけるピクセルのサイズ
		float2		frustum_size;	//距離1におけるサイズ
		float2		reserved0;
		float3x4	view_mat;
		float4x4	proj_mat;
		float4x4	view_proj_mat;
		float4x4	inv_view_proj_mat;
		float4x4	prev_inv_view_proj_mat;
	};
	auto p_scene_info = gp_render_device->create_temporary_cbuffer(sizeof(scene_info_t));
	context.set_pipeline_resource("scene_info", *p_scene_info, true);

	const uint frame_count = gp_render_device->frame_count();
	const float jitter_x = (van_der_corput_sequence(frame_count % 64, 2) - 0.5f) / screen_size.x * jitter_scale;
	const float jitter_y = (van_der_corput_sequence(frame_count % 64, 2) - 0.5f) / screen_size.y * jitter_scale;
	const auto jitter_mat = translate(jitter_x, jitter_y, 0);

	auto &scene_info = *p_scene_info->data<scene_info_t>();
	scene_info.screen_size = screen_size;
	scene_info.inv_screen_size.x = 1 / float(screen_size.x);
	scene_info.inv_screen_size.y = 1 / float(screen_size.y);
	scene_info.camera_pos = m_camera.position();
	scene_info.view_mat = m_camera.view_mat();
	scene_info.proj_mat = jitter_mat * m_camera.proj_mat();
	scene_info.view_proj_mat = scene_info.proj_mat * float4x4(scene_info.view_mat);
	scene_info.inv_view_proj_mat = inverse(scene_info.view_proj_mat);
	scene_info.prev_inv_view_proj_mat = m_inv_view_proj_mat;
	m_inv_view_proj_mat = scene_info.inv_view_proj_mat;

	scene_info.frustum_size.y = tan(to_radian(m_camera.fovy() / 2)) * 2;
	scene_info.frustum_size.x = scene_info.frustum_size.y * m_camera.aspect();
	scene_info.pixel_size = scene_info.frustum_size.y / screen_size.y;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//GUI描画＆Present
inline void base_application::composite_gui_and_present(render_context& context, shader_resource_view& hdr_srv, bool r9g9b9e5)
{
	context.set_target_state(*mp_ldr_target);
	context.set_priority(priority_initiaize);
	context.clear_render_target_view(*mp_ldr_color_rtv, float4(0.0f));
	context.set_priority(priority_render_gui);
	gp_gui_manager->render(*gp_render_device, context);

	if(m_present_shaders.has_update())
		m_present_shaders.update();
	else if(m_present_shaders.is_invalid())
		return;

	context.set_priority(priority_present);
	context.set_pipeline_resource("hdr_srv", hdr_srv);
	context.set_pipeline_resource("ldr_srv", *mp_ldr_color_srv);
	context.set_target_state(*mp_present_target[back_buffer_index()]);
	context.set_geometry_state(*mp_dummy_geometry);
	context.set_pipeline_state(*m_present_shaders.get(r9g9b9e5 ? "prepare_present_r9g9b9e5" : "prepare_present"));
	context.draw(3, 1, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//グリッドを描画
//inline void base_application::draw_grid(render_context& context, const float3& color, const float min, const float max, const int subdiv)
//{
//	depthがいる...
//
//
//	if(m_dev_draw_shaders.has_update())
//		m_dev_draw_shaders.update();
//	else if(m_dev_draw_shaders.is_invalid())
//		return;
//
//	context.push_priority(priority_render_gui + 1);
//	context.set_target_state(*mp_ldr_target);
//	context.set_geometry_state(*mp_dummy_geometry);
//	context.draw(2 * (2 + subdiv), 2, 0);
//	context.pop_priority();
//}

///////////////////////////////////////////////////////////////////////////////////////////////////

//3次元軸を描画
//inline void base_application::draw_axes(render_context& context, const float color_scale)
//{
//}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
