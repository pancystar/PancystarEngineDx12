#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"
#include<LoaderHelpers.h>
#include<DDSTextureLoader.h>
#include<WICTextureLoader.h>
namespace PancystarEngine
{
	uint32_t MyCountMips(uint32_t width, uint32_t height);
	inline bool CheckIfDepthStencil(DXGI_FORMAT fmt);
	inline void MyAdjustPlaneResource(
		_In_ DXGI_FORMAT fmt,
		_In_ size_t height,
		_In_ size_t slicePlane,
		_Inout_ D3D12_SUBRESOURCE_DATA& res);
	HRESULT MyFillInitData(_In_ size_t width,
		_In_ size_t height,
		_In_ size_t depth,
		_In_ size_t mipCount,
		_In_ size_t arraySize,
		_In_ size_t numberOfPlanes,
		_In_ DXGI_FORMAT format,
		_In_ size_t maxsize,
		_In_ size_t bitSize,
		_In_reads_bytes_(bitSize) const uint8_t* bitData,
		_Out_ size_t& twidth,
		_Out_ size_t& theight,
		_Out_ size_t& tdepth,
		_Out_ size_t& skipMip,
		std::vector<D3D12_SUBRESOURCE_DATA>& initData);
	class PancyBasicTexture : public PancystarEngine::PancyBasicVirtualResource
	{
		bool                             if_cube_map;   //是否是cubemap(仅dds有效)
		bool                             if_gen_mipmap; //是否为无mipmap的纹理创建mipmap
		bool                             if_force_srgb; //是否强制转换为线性空间纹理
		int                              max_size;      //纹理最大大小
		D3D12_RESOURCE_DESC              texture_desc;  //纹理格式
		VirtualMemoryPointer             tex_data;      //纹理数据指针
	public:
		PancyBasicTexture(
			std::string desc_file_in,
			bool if_gen_mipmap_in = false,
			bool if_force_srgb_in = false,
			int max_size_in = 0);
	private:
		PancystarEngine::EngineFailReason InitResource(const std::string &resource_desc_file);
		PancystarEngine::EngineFailReason LoadPictureFromFile(const std::string &picture_path_file);
		PancystarEngine::EngineFailReason BuildEmptyPicture(const std::string &picture_desc_file);
		std::string GetFileTile(const std::string &data_input);
		PancystarEngine::EngineFailReason BuildTextureResource(
			const D3D12_RESOURCE_DIMENSION &resDim,
			const size_t &width,
			const size_t &height,
			const size_t &depth,
			const size_t &mipCount,
			const size_t &arraySize,
			DXGI_FORMAT &format,
			const D3D12_RESOURCE_FLAGS &resFlags,
			const unsigned int &loadFlags
		);
		PancystarEngine::EngineFailReason UpdateTextureResourceAndWait(const std::vector<D3D12_SUBRESOURCE_DATA> &subresources);
		bool CheckIfJson(const std::string &path_name);
		inline bool CheckIfPow2(int32_t input) 
		{
			if (input == 32 || input == 64 || input == 128 || input == 256 || input == 512 || input == 1024 || input == 2048 || input == 4096) 
			{
				return true;
			}
			return false;
		}
	};




	class PancyTextureControl : public PancystarEngine::PancyBasicResourceControl
	{
	private:
		PancyTextureControl(const std::string &resource_type_name_in);
	public:
		static PancyTextureControl* GetInstance()
		{
			static PancyTextureControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyTextureControl("Texture Resource Control");
			}
			return this_instance;
		}
	private:
		PancystarEngine::EngineFailReason BuildResource(const std::string &desc_file_in, PancyBasicVirtualResource** resource_out);
	};

	
}