#include<iostream> 
#include<windows.h>
#include<math.h>
#include<vector>
#include <fstream>
#include <io.h>
#include<string>
#include<unordered_set>
#include"DirectXTex.h"
using namespace std;
using namespace DirectX;
struct TargaHeader
{
	unsigned char data1[12];
	unsigned short width;
	unsigned short height;
	unsigned char bpp;
	unsigned char data2;
};

HRESULT save_pic_asTGA(char* filename, int width, int height, char *imagedata)
{
	int error, bpp, imageSize;
	FILE* filePtr;
	TargaHeader targaFileHeader;
	error = fopen_s(&filePtr, filename, "wb");
	if (error != 0)
	{
		return E_FAIL;
	}
	//~~~~~~~~~~~~~~~~~~~~~文件头~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	targaFileHeader.width = width;
	targaFileHeader.height = height;
	targaFileHeader.bpp = 32;
	targaFileHeader.data2 = 8;
	for (int i = 0; i < 12; ++i)
	{
		targaFileHeader.data1[i] = 0;
	}
	targaFileHeader.data1[2] = 2;
	fwrite(&targaFileHeader, sizeof(TargaHeader), 1, filePtr);
	//~~~~~~~~~~~~~~~~~~~~~~文件数据~~~~~~~~~~~~~~~~~~~~~~~~~~~

	imageSize = width * height * 4;
	fwrite(imagedata, 1, imageSize, filePtr);

	error = fclose(filePtr);
	if (error != 0)
	{
		return E_FAIL;
	}
	return S_OK;
}

void getFiles(string path, std::vector<string>& files)
{
	//文件句柄  
	__int64   hFile = 0;
	//文件信息  
	struct __finddata64_t fileinfo;
	string p;
	if ((hFile = _findfirst64(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//如果是目录,迭代之  
			//如果不是,加入列表  
			if ((fileinfo.attrib &  _A_SUBDIR))
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					getFiles(p.assign(path).append("\\").append(fileinfo.name), files);
			}
			else
			{
				files.push_back(p.assign(path).append("\\").append(fileinfo.name));
			}
		} while (_findnext64(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}
bool checkTypetga(string path)
{
	if (path[path.length() - 1] == 'a' && path[path.length() - 2] == 'g' && path[path.length() - 3] == 't')
	{
		return true;
	}
	return false;
}
std::string change_name(std::string file_in, std::string tail)
{
	int length = file_in.size();
	for (int i = file_in.size() - 1; i >= 0; --i)
	{
		length -= 1;
		if (file_in[i] == '.')
		{
			break;
		}
	}
	if (length > 0)
	{
		return file_in.substr(0, length) + tail;
	}
	else
	{
		return "";
	}
}
std::string find_tail(std::string file_in) 
{
	int st_pos = 0;
	for (int i = 0; i < file_in.size(); ++i) 
	{
		if (file_in[i] == '_') 
		{
			st_pos = i + 1;
		}
	}
	return file_in.substr(st_pos, file_in.size() - st_pos);
}
std::string find_need_name(std::string file_in, std::string name_middle)
{
	int st_pos = 0;
	for (int i = 0; i < file_in.size(); ++i)
	{
		if (file_in[i] == '.')
		{
			st_pos = i;
		}
	}
	return file_in.substr(0, st_pos) + name_middle + file_in.substr(st_pos, file_in.size() - st_pos);
}
int main()
{
	std::unordered_set<std::string> mask_tail;
	//添加需要屏蔽的纹理
	mask_tail.insert("Opacity.bmp");
	mask_tail.insert("SubsurfaceAmount.bmp");
	mask_tail.insert("SubsurfaceColor.bmp");
	vector<string> file_name;
	int count = 0;
	getFiles("pic_dir", file_name);
	char *rec_data;
	CoInitialize(NULL);
	for (int i = 0; i < file_name.size(); ++i)
	{
		std::string now_tail = find_tail(file_name[i]);
		if (mask_tail.find(now_tail) != mask_tail.end()) 
		{
			//该纹理后缀被屏蔽
			continue;
		}

		char szStr[256];
		strcpy(szStr, file_name[i].c_str());
		WCHAR wszClassName[256];
		memset(wszClassName, 0, sizeof(wszClassName));
		MultiByteToWideChar(CP_ACP, 0, szStr, strlen(szStr) + 1, wszClassName, sizeof(wszClassName) / sizeof(wszClassName[0]));
		TexMetadata metadata;
		ScratchImage *tgadata = new ScratchImage();
		HRESULT hr;
		if (checkTypetga(szStr))
		{
			LoadFromTGAFile(wszClassName, &metadata, *tgadata);
		}
		else
		{
			hr = LoadFromWICFile(wszClassName, WIC_FLAGS_FORCE_RGB, &metadata, *tgadata);
		}
		tgadata->GetImages();
		char rec_index[] = "ans_dir";
		for (int j = 0; j < 7; ++j)
		{
			szStr[j] = rec_index[j];
		}
		string out_data = change_name(szStr, ".dds");
		memset(wszClassName, 0, sizeof(wszClassName));
		MultiByteToWideChar(CP_ACP, 0, out_data.c_str(), strlen(out_data.c_str()) + 1, wszClassName, sizeof(wszClassName) / sizeof(wszClassName[0]));

		//半透明通道处理
		std::string opacity_name = find_need_name(file_name[i], "_Opacity");
		char opacity_Str[256];
		strcpy(opacity_Str, opacity_name.c_str());
		WCHAR opacity_Name[256];
		memset(opacity_Name, 0, sizeof(opacity_Name));
		MultiByteToWideChar(CP_ACP, 0, opacity_Str, strlen(opacity_Str) + 1, opacity_Name, sizeof(opacity_Name) / sizeof(opacity_Name[0]));
		TexMetadata metadata_opacity;
		ScratchImage *tgadata_opacity = new ScratchImage();
		if (checkTypetga(opacity_name))
		{
			hr = LoadFromTGAFile(opacity_Name, &metadata_opacity, *tgadata_opacity);
		}
		else
		{
			hr = LoadFromWICFile(opacity_Name, WIC_FLAGS_FORCE_RGB, &metadata_opacity, *tgadata_opacity);
		}
		if (!FAILED(hr)) 
		{
			int a = 0;
			auto alpha_opacity = tgadata_opacity->GetImages();
			auto texture_out = tgadata->GetImages();
			int image_width = tgadata->GetImages()->width;
			int image_height = tgadata->GetImages()->height;
			for (int i = 0; i < image_height; ++i)
			{
				for (int j = 0; j < image_width; ++j)
				{
					texture_out->pixels[i*image_width*4 + j*4 + 3] = alpha_opacity->pixels[i*image_width * 4 + j * 4 + 1];
				}
			}
		}
		hr = DirectX::SaveToDDSFile(*tgadata->GetImages(), 0, wszClassName);
	}
	return 0;
}