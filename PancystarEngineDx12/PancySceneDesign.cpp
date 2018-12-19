#include"PancySceneDesign.h"
SceneRoot::SceneRoot()
{
	back_buffer_num = 2;
	If_dsv_loaded = false;
	time_game = 0;
	scene_center_pos = DirectX::XMFLOAT3(0, 0, 0);
	for (int i = 0; i < back_buffer_num; ++i)
	{
		pancy_object_id rec_res;
		ResourceViewPointer rec_res_view;
		Default_depthstencil_buffer.push_back(rec_res);
		Default_depthstencil_view.push_back(rec_res_view);
	}
}
PancystarEngine::EngineFailReason SceneRoot::Create(int32_t width_in, int32_t height_in)
{
	PancystarEngine::EngineFailReason check_error;
	check_error = ResetScreen(width_in, height_in);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = Init();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SceneRoot::ResetScreen(int32_t width_in, int32_t height_in)
{
	PancystarEngine::EngineFailReason check_error;
	Scene_width = width_in;
	Scene_height = height_in;
	std::vector<D3D12_HEAP_FLAGS> heap_flags;
	//创建新的屏幕空间纹理格式
	D3D12_RESOURCE_DESC default_tex_RGB_desc;
	D3D12_RESOURCE_DESC default_tex_SRGB_desc;
	D3D12_RESOURCE_DESC default_tex_float_desc;
	D3D12_RESOURCE_DESC depth_stencil_desc;
	default_tex_RGB_desc.Alignment = 0;
	default_tex_RGB_desc.DepthOrArraySize = 1;
	default_tex_RGB_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	default_tex_RGB_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	default_tex_RGB_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	default_tex_RGB_desc.Height = Scene_height;
	default_tex_RGB_desc.Width = Scene_width;
	default_tex_RGB_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	default_tex_RGB_desc.MipLevels = 1;
	default_tex_RGB_desc.SampleDesc.Count = 1;
	default_tex_RGB_desc.SampleDesc.Quality = 0;
	std::string subres_name;
	//创建rgb8类型的窗口大小纹理格式
	heap_flags.clear();
	heap_flags.push_back(D3D12_HEAP_FLAG_DENY_BUFFERS);
	heap_flags.push_back(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES);
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->BuildTextureTypeJson(default_tex_RGB_desc, 1, D3D12_HEAP_TYPE_DEFAULT,heap_flags, D3D12_RESOURCE_STATE_COMMON, subres_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::string RGB8_file_data = "screentarget\\screen_" + std::to_string(width_in) + "_" + std::to_string(height_in) + "_RGB8.json";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(RGB8_file_data))
	{
		Json::Value json_data_out;
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFSRV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfRTV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFUAV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFDSV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "SubResourceFile", subres_name);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, RGB8_file_data);
		//将文件标记为已经创建
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(RGB8_file_data);
	}
	//创建srgb8类型的窗口大小纹理格式
	default_tex_SRGB_desc = default_tex_RGB_desc;
	default_tex_SRGB_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->BuildTextureTypeJson(default_tex_SRGB_desc, 1, D3D12_HEAP_TYPE_DEFAULT, heap_flags, D3D12_RESOURCE_STATE_COMMON, subres_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::string SRGB8_file_data = "screentarget\\screen_" + std::to_string(width_in) + "_" + std::to_string(height_in) + "_SRGB8.json";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(SRGB8_file_data))
	{
		Json::Value json_data_out;
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFSRV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfRTV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFUAV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFDSV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "SubResourceFile", subres_name);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, SRGB8_file_data);
		//将文件标记为已经创建
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(SRGB8_file_data);
	}
	//创建rgb16类型的窗口大小纹理格式
	default_tex_float_desc = default_tex_RGB_desc;
	default_tex_float_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->BuildTextureTypeJson(default_tex_float_desc, 1, D3D12_HEAP_TYPE_DEFAULT, heap_flags, D3D12_RESOURCE_STATE_COMMON, subres_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::string RGB16_file_data = "screentarget\\screen_" + std::to_string(width_in) + "_" + std::to_string(height_in) + "_RGB16.json";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(RGB16_file_data))
	{
		Json::Value json_data_out;
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFSRV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfRTV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFUAV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFDSV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "SubResourceFile", subres_name);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, RGB16_file_data);
		//将文件标记为已经创建
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(RGB16_file_data);
	}
	//创建深度模板缓冲区
	depth_stencil_desc = default_tex_RGB_desc;
	depth_stencil_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	heap_flags.clear();
	heap_flags.push_back(D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES);
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->BuildTextureTypeJson(depth_stencil_desc, 1, D3D12_HEAP_TYPE_DEFAULT, heap_flags, D3D12_RESOURCE_STATE_COMMON, subres_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::string d24s8_file_data = "screentarget\\screen_" + std::to_string(width_in) + "_" + std::to_string(height_in) + "_DSV.json";
	if (!PancystarEngine::FileBuildRepeatCheck::GetInstance()->CheckIfCreated(d24s8_file_data))
	{
		Json::Value json_data_out;
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfFromFile", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFSRV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IfRTV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFUAV", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "IFDSV", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "SubResourceFile", subres_name);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, d24s8_file_data);
		//将文件标记为已经创建
		PancystarEngine::FileBuildRepeatCheck::GetInstance()->AddFileName(d24s8_file_data);
	}
	//删除旧的深度模板缓冲区
	if (If_dsv_loaded)
	{
		for (int i = 0; i < back_buffer_num; ++i)
		{
			PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(Default_depthstencil_buffer[i]);
		}
	}
	for (int i = 0; i < back_buffer_num; ++i)
	{
		//加载深度模板缓冲区
		std::string depth_stencil_use = "screentarget\\screen_" + std::to_string(width_in) + "_" + std::to_string(height_in) + "_DSV.json";
		auto check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(depth_stencil_use, Default_depthstencil_buffer[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//创建深度模板缓冲区描述符
		SubMemoryPointer tex_resource_data;
		D3D12_DEPTH_STENCIL_VIEW_DESC DSV_desc;
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[i], tex_resource_data);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetDSVDesc(Default_depthstencil_buffer[i], DSV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string dsv_descriptor_name = "json\\descriptor_heap\\DSV_1_descriptor_heap.json";
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(dsv_descriptor_name, Default_depthstencil_view[i].resource_view_pack_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		Default_depthstencil_view[i].resource_view_offset_id = 0;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildDSV(Default_depthstencil_view[i], tex_resource_data, DSV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}

	//D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	check_error = ScreenChange();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}

LRESULT CALLBACK engine_windows_main::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:                // 键盘按下消息
		if (wParam == VK_ESCAPE)    // ESC键
			DestroyWindow(hwnd);    // 销毁窗口, 并发送一条WM_DESTROY消息
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
	case WM_SIZE:

		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
engine_windows_main::engine_windows_main(HINSTANCE hInstance_need, HINSTANCE hPrevInstance_need, PSTR szCmdLine_need, int iCmdShow_need)
{
	hwnd = NULL;
	hInstance = hInstance_need;
	hPrevInstance = hPrevInstance_need;
	szCmdLine = szCmdLine_need;
	iCmdShow = iCmdShow_need;
	window_width = 1280;
	window_height = 720;
}
HRESULT engine_windows_main::game_create(SceneRoot   *new_scene_in)
{
	//填充窗口类型
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = TEXT("pancystar_engine");

	//取消dpi对游戏的缩放
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"),
			TEXT("pancystar_engine"), MB_ICONERROR);
		return E_FAIL;
	}
	//获取渲染窗口真正的大小
	RECT R = { 0, 0, window_width, window_height };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;
	//创建窗口
	hwnd = CreateWindow(TEXT("pancystar_engine"),
		TEXT("pancystar_engine"),
		WS_DLGFRAME | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		hInstance,
		NULL);
	if (hwnd == NULL)
	{
		return E_FAIL;
	}
	RECT new_info;
	GetWindowRect(hwnd, &new_info);

	//创建directx设备
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyDx12DeviceBasic::SingleCreate(hwnd, window_width, window_height);
	if (!check_error.CheckIfSucceed())
	{
		return E_FAIL;
	}
	//注册单例
	PancyShaderControl::GetInstance();
	PancyRootSignatureControl::GetInstance();
	PancyEffectGraphic::GetInstance();
	PancyJsonTool::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	PancyDescriptorHeapControl::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	SubresourceControl::GetInstance();
	PancyInput::SingleCreate(hwnd, hInstance);
	PancyCamera::GetInstance();
	//创建线程池管理
	check_error = ThreadPoolGPUControl::SingleCreate();
	if (!check_error.CheckIfSucceed())
	{
		return E_FAIL;
	}
	//创建场景
	new_scene = new_scene_in;
	//new_scene = new scene_test_simple();

	check_error = new_scene->Create(window_width, window_height);
	if (!check_error.CheckIfSucceed())
	{
		return E_FAIL;
	}
	ShowWindow(hwnd, SW_SHOW);                    // 将窗口显示到桌面上。
	UpdateWindow(hwnd);                           // 刷新一遍窗口（直接刷新，不向windows消息循环队列做请示）。
	return S_OK;
}
HRESULT engine_windows_main::game_loop()
{
	//游戏循环
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			//new_scene->Render();
			new_scene->Update(0);
			new_scene->Display();
			TranslateMessage(&msg);//消息转换
			DispatchMessage(&msg);//消息传递给窗口过程函数
		}
		else
		{
			//new_scene->Render();
			new_scene->Update(0);
			new_scene->Display();
		}
	}
	return S_OK;
}
WPARAM engine_windows_main::game_end()
{
	delete new_scene;
	delete PancyDx12DeviceBasic::GetInstance();
	PancystarEngine::EngineFailLog::GetInstance()->PrintLogToconsole();
	delete PancystarEngine::EngineFailLog::GetInstance();
	delete ThreadPoolGPUControl::GetInstance();
	delete PancyShaderControl::GetInstance();
	delete PancyRootSignatureControl::GetInstance();
	delete PancyEffectGraphic::GetInstance();
	delete PancyJsonTool::GetInstance();
	delete MemoryHeapGpuControl::GetInstance();
	delete PancyDescriptorHeapControl::GetInstance();
	delete PancystarEngine::PancyTextureControl::GetInstance();
	delete SubresourceControl::GetInstance();
	delete PancystarEngine::FileBuildRepeatCheck::GetInstance();
	delete PancyInput::GetInstance();
	delete PancyCamera::GetInstance();
	return msg.wParam;
}