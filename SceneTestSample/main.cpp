#include"TestScene.h"


//windows函数的入口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCmdLine, int iCmdShow)
{
	//_CrtSetBreakAlloc(286831);
	engine_windows_main *engine_main = new engine_windows_main(hInstance, hPrevInstance, szCmdLine, iCmdShow);
	auto new_scene = new scene_test_simple();
	HRESULT hr = engine_main->game_create(new_scene);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailLog::GetInstance()->PrintLogToconsole();
		return 0;
	}
	engine_main->game_loop();
	auto msg_end = engine_main->game_end();
	delete engine_main;
	if (InputLayoutDesc::GetInstance() != NULL)
	{
		delete InputLayoutDesc::GetInstance();
	}
#ifdef CheckWindowMemory
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

	//_CrtDumpMemoryLeaks();

	return static_cast<int>(msg_end);
}