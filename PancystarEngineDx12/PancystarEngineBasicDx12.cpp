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
EngineFailReasonMessage::EngineFailReasonMessage()
{
	std::string a;
	if_succeed = true;
	windows_result = S_OK;
	failed_reason = "";
	log_type = LOG_MESSAGE_SUCCESS;
}
EngineFailReasonMessage::EngineFailReasonMessage(HRESULT windows_result_in, std::string failed_reason_in, LogMessageType log_type_in)
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
		ShowFailedReason();
		if_succeed = false;
	}
}
void EngineFailReasonMessage::ShowFailedReason()
{
	MessageBox(0, failed_reason.GetUnicodeString().c_str(), L"error", MB_OK);
}
//log记录
EngineFailLog* EngineFailLog::this_instance = NULL;
void EngineFailLog::AddLog(const std::string& log_source, const EngineFailReasonMessage& engine_error_message, EngineFailReason& error_log)
{
	PancyBasicLog new_log;
	new_log.log_source = log_source;
	new_log.fail_reason = engine_error_message;
	log_save_list.push_back(new_log);
	error_log.if_succeed = engine_error_message.CheckIfSucceed();
	error_log.log_id = static_cast<pancy_object_id>(log_save_list.size()) - 1;
}
void PancystarEngine::BuildDebugLog(
	const HRESULT& windows_result,
	const std::string& error_reason,
	const std::string& file_name,
	const std::string& function_name,
	const pancy_object_id& line,
	const LogMessageType& log_type,
	EngineFailReason& log_turn
)
{
	PancyBasicLog new_log;
	std::string new_log_source = "file: " + file_name + " function: " + function_name + " line " + std::to_string(line);
	EngineFailReasonMessage new_message(windows_result, new_log_source, log_type);
	EngineFailLog::GetInstance()->AddLog(new_log_source, new_message, log_turn);
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
void PancystarEngine::DivideFilePath(const std::string &full_file_name_in, std::string &file_path_out, std::string &file_name_out, std::string &file_tail_out)
{
	//获取文件后缀名
	size_t length_name = full_file_name_in.size();
	while (full_file_name_in[length_name - 1] != '.') 
	{
		length_name -= 1;
	}
	for (size_t i = length_name; i < full_file_name_in.size(); ++i) 
	{
		file_tail_out += full_file_name_in[i];
	}
	length_name -= 1;
	//处理存储文件的文件名
	std::string file_root_name = full_file_name_in.substr(0, length_name);
	int32_t st_pos = 0;
	for (int32_t i = 0; i < file_root_name.size(); ++i)
	{
		if (file_root_name[i] == '\\' || file_root_name[i] == '/')
		{
			st_pos = i + 1;
		}
	}
	if (st_pos < file_root_name.size())
	{
		file_name_out = file_root_name.substr(st_pos, file_root_name.size() - st_pos);
	}
	else
	{
		file_name_out = "";
	}
	//获取文件的路径名
	file_path_out = file_root_name.substr(0, st_pos);
}
pancy_resource_size PancystarEngine::SizeAligned(const pancy_resource_size &size_in, const pancy_resource_size &size_aligned_in)
{
	pancy_resource_size out_size = size_in;
	if (size_in % size_aligned_in != 0) 
	{
		auto size_scal = size_in / size_aligned_in;
		out_size = (size_scal + 1) * size_aligned_in;
	}
	return out_size;
}
//文件加载判重
static FileBuildRepeatCheck* this_instance = NULL;
