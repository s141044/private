
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace nn{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace render{

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace d3d12{

///////////////////////////////////////////////////////////////////////////////////////////////////
//gui_manager
///////////////////////////////////////////////////////////////////////////////////////////////////

//コンストラクタ
inline gui_manager::gui_manager(const HWND hwnd) : m_hwnd(hwnd)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//初期化
inline void gui_manager::initialize(iface::render_device& iface_device, const texture_format rtv_format, const uint buffer_count)
{
	auto& device = static_cast<d3d12::render_device&>(iface_device);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
		
	// Setup Platform/Renderer backends
	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = device.get_impl();
	init_info.CommandQueue = device.get_command_queue().get_impl();
	init_info.NumFramesInFlight = buffer_count;
	init_info.RTVFormat = DXGI_FORMAT(rtv_format); // Or your render target format.
	init_info.UserData = &device;

	// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
	// The example_win32_directx12/main.cpp application include a simple free-list based allocator.
	init_info.SrvDescriptorHeap = device.get_descriptor_heap().get_impl();
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* p_info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
	{
		auto* p_device = static_cast<render_device*>(p_info->UserData);
		auto handle = p_device->get_descriptor_heap().allocate_bindless();
		*out_cpu_handle = handle.cpu_start;
		*out_gpu_handle = handle.gpu_start;
	};
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* p_info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
	{
		auto *p_device = static_cast<render_device*>(p_info->UserData);
		p_device->get_descriptor_heap().deallocate_bindless(cpu_handle);
	};

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX12_Init(&init_info);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//終了処理
inline void gui_manager::finalize()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//新規フレーム開始
inline void gui_manager::new_frame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	//ImGui::ShowDemoWindow();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//描画
inline void gui_manager::render(iface::render_device& device, render_context& context)
{
	auto build_command = [&]()
	{
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), static_cast<d3d12::render_device&>(device).get_command_list().get_impl());
	};
	context.call_function(build_command, true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//ウィンドウメッセージ処理
inline bool gui_manager::wnd_proc(const UINT msg, const WPARAM wp, const LPARAM lp)
{
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	return ImGui_ImplWin32_WndProcHandler(m_hwnd, msg, wp, lp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace d3d12

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace render

///////////////////////////////////////////////////////////////////////////////////////////////////

} //namespace nn

///////////////////////////////////////////////////////////////////////////////////////////////////
