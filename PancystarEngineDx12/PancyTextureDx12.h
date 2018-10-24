#pragma once
#include"PancyResourceBasic.h"
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
		D3D12_SHADER_RESOURCE_VIEW_DESC  SRV_need;      //SRV访问视图
		D3D12_RENDER_TARGET_VIEW_DESC    RTV_need;      //RTV访问视图
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAV_need;      //UAV访问视图
		ComPtr<ID3D12DescriptorHeap>     SRV_heap_need; //SRV描述符堆
		ComPtr<ID3D12DescriptorHeap>     RTV_heap_need; //RTV描述符堆
		ComPtr<ID3D12DescriptorHeap>     UAV_heap_need; //UAV描述符堆
	public:
		PancyBasicTexture(
			std::string desc_file_in,
			bool if_gen_mipmap_in = false,
			bool if_force_srgb_in = false,
			int max_size_in = 0);
	private:
		PancystarEngine::EngineFailReason InitResource(std::string resource_desc_file);
		PancystarEngine::EngineFailReason BuildSRVFromResource();
		PancystarEngine::EngineFailReason LoadPictureFromFile(std::string picture_path_file);
		PancystarEngine::EngineFailReason BuildEmptyPicture(std::string picture_desc_file);
		std::string GetFileTile(const std::string &data_input);
		PancystarEngine::EngineFailReason BuildTextureResource(
			D3D12_RESOURCE_DIMENSION resDim,
			size_t width,
			size_t height,
			size_t depth,
			size_t mipCount,
			size_t arraySize,
			DXGI_FORMAT format,
			D3D12_RESOURCE_FLAGS resFlags,
			unsigned int loadFlags
		);
	};
	PancystarEngine::EngineFailReason PancyBasicTexture::BuildSRVFromResource()
	{
		auto resource = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(tex_data);

	}
	class PancyTextureControl 
	{
	};
	
}