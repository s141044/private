
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////
//bindless_geometry
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline bindless_geometry::bindless_geometry(const geometry_state &gs, const uint offsets[8])
{
	struct data_t
	{
		uint32_t	num_vbs;
		uint32_t	ib_handle;
		uint32_t	vb_handles[8];
		uint32_t	offsets[8];
		uint8_t		strides[8];
	};
	
	data_t data;
	data.num_vbs = gs.num_vbs();
	data.ib_handle = invalid_bindless_handle;
	if(gs.has_index_buffer())
	{
		mp_ib_srv = gp_render_device->create_shader_resource_view(gs.index_buffer(), buffer_srv_desc(gs.index_buffer(), 0, -1, true));
		gp_render_device->register_bindless(*mp_ib_srv);
		data.ib_handle = mp_ib_srv->bindless_handle();
	}
	for(uint i = 0; i < gs.num_vbs(); i++)
	{
		uint j;
		for(j = i; j > 0; j--)
		{
			if(&gs.vertex_buffer(i) == &gs.vertex_buffer(j - 1))
				break;
		}
		if(j != 0)
			mp_vb_srv[i] = mp_vb_srv[j - 1];
		else
		{
			mp_vb_srv[i] = gp_render_device->create_shader_resource_view(gs.vertex_buffer(i), buffer_srv_desc(gs.vertex_buffer(i), 0, -1, true));
			gp_render_device->register_bindless(*mp_vb_srv[i]);
		}
		data.vb_handles[i] = mp_vb_srv[i]->bindless_handle();
		data.offsets[i] = uint32_t(gs.offset(i) + offsets[i]);
		data.strides[i] = uint8_t(gs.stride(i));
	}
	for(uint i = gs.num_vbs(); i < 8; i++)
		data.vb_handles[i] = invalid_bindless_handle;

	mp_buf = gp_render_device->create_byteaddress_buffer(sizeof(data), resource_flag_allow_shader_resource, &data);
	mp_srv = gp_render_device->create_shader_resource_view(*mp_buf, buffer_srv_desc(*mp_buf));
	gp_render_device->register_bindless(*mp_srv);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//デストラクタ
inline bindless_geometry::~bindless_geometry()
{
	gp_render_device->unregister_bindless(*mp_srv);

	if(mp_ib_srv)
		gp_render_device->unregister_bindless(*mp_ib_srv);

	for(uint i = 0; i < 8; i++)
	{
		if(mp_vb_srv[i] && (mp_vb_srv[i]->bindless_handle() != invalid_bindless_handle))
			gp_render_device->unregister_bindless(*mp_vb_srv[i]);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
