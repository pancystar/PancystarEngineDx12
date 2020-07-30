#pragma once
#include"PancystarEngineBasicDx12.h"
using namespace std;
class PancyInput
{
	IDirectInput8 *pancy_dinput;           //DirectInput的设备接口
	IDirectInputDevice8 *dinput_keyboard;  //键盘设备接口
	IDirectInputDevice8 *dinput_mouse;     //鼠标设备接口
	char                       key_buffer[256];   //键盘按键信息的缓存
	DIMOUSESTATE               mouse_buffer;      //鼠标控制信息的缓存
private:
	PancyInput();         //构造函数
public:
	static PancyInput *pancy_input_pInstance;
	static PancystarEngine::EngineFailReason SingleCreate(HWND hwnd, HINSTANCE hinst)
	{
		if (pancy_input_pInstance != NULL)
		{
			
			PancystarEngine::EngineFailReason error_message;
			PancyDebugLogError(E_FAIL, "the d3d input instance have been created before",error_message);
			
			return error_message;
		}
		else
		{
			pancy_input_pInstance = new PancyInput();
			PancystarEngine::EngineFailReason check_error = pancy_input_pInstance->Init(hwnd, hinst);
			return check_error;
		}
	}
	static PancyInput * GetInstance()
	{
		return pancy_input_pInstance;
	}
	~PancyInput();
	void GetInput();                                                       //获取外设输入
	bool CheckKeyboard(int key_value);                                     //检测键盘上的某个键按下与否
	bool CheckMouseDown(int mouse_value);                                  //检测鼠标上的某个键按下与否
	float MouseMove_X();                                                     //获取鼠标在x轴的移动量
	float MouseMove_Y();                                                     //获取鼠标在y轴的移动量
	float MouseMove_Z();                                                     //获取鼠标在z轴的移动量
	PancystarEngine::EngineFailReason Init(HWND hwnd, HINSTANCE hinst);
private:
	PancystarEngine::EngineFailReason DinputClear(HWND hwnd,DWORD keyboardCoopFlags, DWORD mouseCoopFlags);//初始化函数
};