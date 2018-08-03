#pragma once
//windows头文件
#include<Windows.h>
#include<d3d12.h>
#include<DirectXMath.h>
//#include<d3d12shader.h>
#include<d3dcompiler.h>
#include"d3dx12.h"
#include<DXGI1_6.h>
#include<ShellScalingApi.h>
#include <wrl/client.h>
#include<comdef.h>
#pragma comment(lib, "Shcore.lib")
using Microsoft::WRL::ComPtr;
// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
//STL容器
#include <iostream>
#include <algorithm>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <vector>
#include <queue>
//c++运行时
#include <string>
#include <fstream>
#include <typeinfo>
//内存泄漏检测
#define CheckWindowMemory
#ifdef CheckWindowMemory
   #include <crtdbg.h>
    //#ifdef _DEBUG
    //#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)  
    //#endif
#endif
namespace PancystarEngine
{
	enum LogMessageType 
	{
		LOG_MESSAGE_SUCCESS = 0,
		LOG_MESSAGE_WARNING = 1,
		LOG_MESSAGE_ERROR = 2
	};
	//观察者
	class window_size_observer
	{
	public:
		virtual void update_windowsize(int wind_width_need, int wind_height_need) = 0;
	};
	//被观察对象
	class window_size_subject
	{
	protected:
		std::list<window_size_observer*> observer_list;
	public:
		virtual void attach(window_size_observer*) = 0;
		virtual void detach(window_size_observer*) = 0;
		virtual void notify(int wind_width_need, int wind_height_need) = 0;
	};
	//各种类字符串处理类
	class PancyString 
	{
		std::string ascii_string;
		std::wstring unicode_string;
	public:
		PancyString();
		PancyString(std::string string_in);
		PancyString(std::wstring string_in);
		PancyString& operator=(_In_z_ const char * const _Ptr);
		PancyString& operator=(_In_z_ const std::string _Str);
		PancyString& operator=(_In_z_ const wchar_t * const _Ptr);
		PancyString& operator=(_In_z_ const std::wstring _Str);
		std::string GetAsciiString() 
		{
			return ascii_string;
		}
		std::wstring GetUnicodeString() 
		{
			return unicode_string;
		};
	private:
		void StringToWstring();
		void WstringToString();
	};
	
	//错误返回信息
	class EngineFailReason
	{
		bool if_succeed;
		HRESULT windows_result;
		PancyString failed_reason;
		LogMessageType log_type;
	public:
		EngineFailReason();
		EngineFailReason(HRESULT windows_result_in, std::string failed_reason_in, LogMessageType log_type_in = LOG_MESSAGE_ERROR);
		PancyString GetFailReason() 
		{
			return failed_reason;
		};
		LogMessageType GetFailedType()
		{
			return log_type;
		};
		bool CheckIfSucceed() 
		{
			return if_succeed; 
		};
	private:
		void ShowFailedReason();
	};

	struct PancyBasicLog 
	{
		std::string log_source;
		EngineFailReason fail_reason;
	};
	class EngineFailLog 
	{
		std::string log_file_name;
		std::vector<PancyBasicLog> log_save_list;
	private:
		EngineFailLog() 
		{
		};
	public:
		static EngineFailLog* this_instance;
		static EngineFailLog* GetInstance()
		{
			
			if (this_instance == NULL)
			{
				this_instance = new EngineFailLog();
			}
			return this_instance;
		}
		static void Release()
		{
			if (this_instance != NULL) 
			{
				delete this_instance;
			}
		}
		//log信息(log来源，log错误记录)
		void AddLog(std::string log_source, EngineFailReason engine_error_message);
		void SaveLogToFile(std::string log_file_name);
		void PrintLogToconsole();
	};
	static EngineFailReason succeed;
}
