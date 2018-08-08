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
#include <atomic>
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

	//渲染资源
	class PancyBasicVirtualResource
	{
	protected:
		uint32_t resource_id;
		std::atomic<int32_t> reference_count;
	public:
		PancyBasicVirtualResource(uint32_t resource_id_in);
		PancystarEngine::EngineFailReason Create();
		void AddReference();
		void DeleteReference();
		inline int32_t GetReference()
		{
			return reference_count;
		}
	protected:
		virtual PancystarEngine::EngineFailReason InitResource() = 0;
	};
	PancyBasicVirtualResource::PancyBasicVirtualResource(uint32_t resource_id_in)
	{
		resource_id = resource_id_in;
		reference_count.store(0);
	}
	void PancyBasicVirtualResource::AddReference()
	{
		reference_count.fetch_add(1);
	}
	void PancyBasicVirtualResource::DeleteReference()
	{
		if (reference_count > 0) 
		{
			reference_count.fetch_sub(1);
		}
		else
		{
			reference_count.store(0);
		}
	}
	PancystarEngine::EngineFailReason PancyBasicVirtualResource::Create()
	{
		auto check_error = InitResource();
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	/*
	template<class T>
	class PancyVirtualResource : public BasicVirtualResource
	{
		T *resource_data;
	public:
		PancyVirtualResource();
		inline T* GetResourceData()
		{
			AddReference();
			return resource_data;
		};
	private:

	};
	*/
	
	class PancyBasicResourceControl 
	{
		std::string resource_type_name;
	protected:
		uint32_t resource_id_self_add;
		std::unordered_map<uint32_t, PancyBasicVirtualResource*> basic_resource_array;
	public:
		PancyBasicResourceControl(std::string resource_type_name_in);
		
		PancystarEngine::EngineFailReason AddResurceReference(const uint32_t &resource_id);
		PancystarEngine::EngineFailReason DeleteResurceReference(const uint32_t &resource_id);
	protected:
		template<class TypeInput>
		PancystarEngine::EngineFailReason BuildResource(const TypeInput &data_input, PancyBasicVirtualResource* data_input);
	};
	PancyBasicResourceControl::PancyBasicResourceControl(std::string resource_type_name_in)
	{
		resource_type_name = resource_type_name_in;
		resource_id_self_add = 0;
	}
	template<class TypeInput>
	PancystarEngine::EngineFailReason PancyBasicResourceControl::BuildResource<TypeInput>(const TypeInput &data_input, PancyBasicVirtualResource* data_input)
	{
		//创建一个新的资源
		if (data_input == NULL) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not build resource with NULL data ID: " + std::to_string(resource_id), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource data in resource control " + resource_type_name, error_message);
			return error_message;
		}
		//插入到资源列表
		basic_resource_array.insert(std::pair<uint32_t, PancyBasicVirtualResource*>(resource_id_self_add, data_input));
		//自增ID号+1
		resource_id_self_add += 1;
		return PancystarEngine::succeed;
	}

	PancystarEngine::EngineFailReason PancyBasicResourceControl::AddResurceReference(const uint32_t &resource_id)
	{
		auto data_now = basic_resource_array.find(resource_id);
		if (data_now == basic_resource_array.end()) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL,"could not find resource ID: " + std::to_string(resource_id),PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add resource reference in resource control " + resource_type_name, error_message);
			return error_message;
		}
		data_now->second->AddReference();
		return PancystarEngine::succeed;
	}
	PancystarEngine::EngineFailReason PancyBasicResourceControl::DeleteResurceReference(const uint32_t &resource_id)
	{
		auto data_now = basic_resource_array.find(resource_id);
		if (data_now == basic_resource_array.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(resource_id), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete resource reference in resource control " + resource_type_name, error_message);
			return error_message;
		}
		data_now->second->DeleteReference();
		//引用计数为0,删除该资源
		if (data_now->second->GetReference() == 0) 
		{
			delete data_now->second;
			basic_resource_array.erase(data_now);
		}
		return PancystarEngine::succeed;
	}

	class PancyBasicResourceView 
	{
		uint32_t resource_id;
		uint32_t resource_view_id;
	public:
		PancyBasicResourceView(const uint32_t &resource_id_in,const uint32_t &resource_view_id_in);

	private:
		template<class TypeInput>
		PancystarEngine::EngineFailReason BuildResource(const TypeInput &data_input);
	};
	PancyBasicResourceView::PancyBasicResourceView(const uint32_t &resource_id_in, const uint32_t &resource_view_id_in)
	{
		resource_id = resource_id_in;
		resource_view_id = resource_view_id_in;
	}
	PancystarEngine::EngineFailReason PancyBasicResourceView::Create()
	{

	}
}

