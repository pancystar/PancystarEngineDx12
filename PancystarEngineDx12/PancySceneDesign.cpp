#include"PancySceneDesign.h"
scene_root::scene_root()
{
	time_game = 0;
	scene_center_pos = DirectX::XMFLOAT3(0, 0, 0);
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
HRESULT engine_windows_main::game_create(scene_root   *new_scene_in)
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