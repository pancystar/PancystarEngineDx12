#include<iostream> 
#include<windows.h>
#include<math.h>
#include<vector>
#include <fstream>
#include <io.h>
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
int main()
{
	vector<string> file_name;
	int count = 0;
	getFiles("pic_dir", file_name);
	char *rec_data;
	CoInitialize(NULL);
	for (int i = 0; i < file_name.size(); ++i)
	{
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


		hr = DirectX::SaveToDDSFile(*tgadata->GetImages(), 0, wszClassName);
		int a = 0;
	}
	return 0;
}