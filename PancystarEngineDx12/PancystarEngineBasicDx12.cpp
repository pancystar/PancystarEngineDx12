#include"PancystarEngineBasicDx12.h"
using namespace PancystarEngine;
//基本字符串处理
PancyString::PancyString()
{
	ascii_string = "";
	unicode_string = L"";
}
PancyString::PancyString(std::string string_in)
{
	ascii_string = string_in;
	StringToWstring();
}
PancyString::PancyString(std::wstring string_in)
{
	unicode_string = string_in;
	WstringToString();
}
PancyString& PancyString::operator=(_In_z_ const char * const _Ptr)
{
	ascii_string = _Ptr;
	StringToWstring();
	return *this;
}
PancyString& PancyString::operator=(_In_z_ const std::string _Str)
{
	ascii_string = _Str;
	StringToWstring();
	return *this;
}
PancyString& PancyString::operator=(_In_z_ const wchar_t * const _Ptr)
{
	unicode_string = _Ptr;
	WstringToString();
	return *this;
}
PancyString& PancyString::operator=(_In_z_ const std::wstring _Str)
{
	unicode_string = _Str;
	WstringToString();
	return *this;
}
void PancyString::StringToWstring()
{
	int nLen = (int)ascii_string.length();
	unicode_string.resize(nLen, L' ');
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)ascii_string.c_str(), nLen, (LPWSTR)unicode_string.c_str(), nLen);
}
void PancyString::WstringToString()
{
	int nLen = (int)unicode_string.length();
	ascii_string.resize(nLen, ' ');
	int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)unicode_string.c_str(), nLen, (LPSTR)ascii_string.c_str(), nLen, NULL, NULL);
}
//错误信息
EngineFailReason::EngineFailReason()
{
	std::string a;
	if_succeed = true;
	windows_result = S_OK;
	failed_reason = "";
	log_type = LOG_MESSAGE_SUCCESS;
}
EngineFailReason::EngineFailReason(HRESULT windows_result_in, std::string failed_reason_in, LogMessageType log_type_in)
{
	windows_result = windows_result_in;
	failed_reason = failed_reason_in;
	log_type = log_type_in;
	if (log_type != LOG_MESSAGE_ERROR)
	{
		if_succeed = true;
	}
	else
	{
		if_succeed = false;
	}
}
void EngineFailReason::ShowFailedReason()
{
	MessageBox(0, failed_reason.GetUnicodeString().c_str(), L"error", MB_OK);
}
//log记录
EngineFailLog* EngineFailLog::this_instance = NULL;
void EngineFailLog::AddLog(std::string log_source, EngineFailReason engine_error_message)
{
	PancyBasicLog new_log;
	new_log.log_source = log_source;
	new_log.fail_reason = engine_error_message;
	log_save_list.push_back(new_log);
}
void EngineFailLog::SaveLogToFile(std::string log_file_name)
{
	std::ofstream location_out;
	location_out.open(log_file_name);
	for (auto log_data = log_save_list.begin(); log_data != log_save_list.end(); ++log_data)
	{
		location_out << log_data->log_source << ": ";
		if (log_data->fail_reason.GetFailedType() == LOG_MESSAGE_SUCCESS)
		{
			location_out << "success" << std::endl;
		}
		else if (log_data->fail_reason.GetFailedType() == LOG_MESSAGE_WARNING)
		{
			location_out << "warning" << ":: " << log_data->fail_reason.GetFailReason().GetAsciiString() << std::endl;
		}
		else if (log_data->fail_reason.GetFailedType() == LOG_MESSAGE_ERROR)
		{
			location_out << "error" << ":: " << log_data->fail_reason.GetFailReason().GetAsciiString() << std::endl;
		}
	}
	location_out.close();
}
void EngineFailLog::PrintLogToconsole()
{
	for (auto log_data = log_save_list.begin(); log_data != log_save_list.end(); ++log_data)
	{
		std::string debug_data = log_data->log_source + ": ";
		if (log_data->fail_reason.GetFailedType() == LOG_MESSAGE_SUCCESS)
		{
			debug_data += "success";
		}
		else if (log_data->fail_reason.GetFailedType() == LOG_MESSAGE_WARNING)
		{
			debug_data += std::string("warning") + ":: " + log_data->fail_reason.GetFailReason().GetAsciiString();
		}
		else if (log_data->fail_reason.GetFailedType() == LOG_MESSAGE_ERROR)
		{
			debug_data += std::string("error") + ":: " + log_data->fail_reason.GetFailReason().GetAsciiString();
		}
		debug_data += "\n";
		PancyString out_debug_string = debug_data;
		OutputDebugString(out_debug_string.GetUnicodeString().c_str());
	}
	log_save_list.clear();
}
//文件加载判重
static FileBuildRepeatCheck* this_instance = NULL;
