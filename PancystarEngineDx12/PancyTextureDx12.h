#pragma once
#include"PancyThreadBasic.h"
#include"PancyBufferDx12.h"

//#define TextureHeapAliaze 16777216
//#define BufferHeapAliaze 4194304
/*
todo:纹理双缓冲
todo:纹理copy queue
*/
enum TextureCompressType 
{
	TEXTURE_COMPRESS_NONE = 0,
	TEXTURE_COMPRESS_BC1,
	TEXTURE_COMPRESS_BC2,
	TEXTURE_COMPRESS_BC3,
	TEXTURE_COMPRESS_BC4,
	TEXTURE_COMPRESS_BC5,
	TEXTURE_COMPRESS_BC6,
	TEXTURE_COMPRESS_BC7
};
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
	//纹理导入参数结构
	enum PancyTextureType 
	{
		Texture_Static_Load = 0,
		Texture_Render_Target
	};
	struct PancyCommonTextureDesc 
	{
		D3D12_HEAP_TYPE heap_type;
		D3D12_HEAP_FLAGS heap_flag_in;
		PancyTextureType texture_type;
		D3D12_RESOURCE_DESC texture_res_desc = {};
		std::string texture_data_file;
		bool if_gen_mipmap; //是否为无mipmap的纹理创建mipmap
		bool if_force_srgb; //是否强制转换为线性空间纹理
		int  max_size;      //纹理最大大小
	};
	class CommonTextureJsonReflect :public PancyJsonReflectTemplate<PancyCommonTextureDesc>
	{
	public:
		CommonTextureJsonReflect();
	private:
		void InitBasicVariable() override;
	};
	//纹理资源
	class PancyBasicTexture : public PancyCommonVirtualResource<PancyCommonTextureDesc>
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC  tex_srv_desc = {};  //纹理访问格式
		bool if_cube_map;   //是否是cubemap(仅dds有效)
		pancy_resource_size subresources_size = 0;
		ResourceBlockGpu *texture_data = nullptr;     //buffer数据指针
	public:
		PancyBasicTexture(const bool &if_could_reload);
		PancystarEngine::EngineFailReason SaveTextureToFile(
			ID3D11Device* pDevice,
			const std::string &file_name,
			bool if_automip = false,
			bool if_compress = false
		);
		//检测当前的资源是否已经被载入GPU
		bool CheckIfResourceLoadFinish() override;;
		inline ResourceBlockGpu *GetGpuResourceData() const
		{
			return texture_data;
		}
		~PancyBasicTexture();
		inline D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc()
		{
			return tex_srv_desc;
		}
	private:
		PancystarEngine::EngineFailReason LoadResoureDataByDesc(const PancyCommonTextureDesc &ResourceDescStruct) override;
		PancystarEngine::EngineFailReason LoadResoureDataByOtherFile(const std::string &file_name, PancyCommonTextureDesc &resource_desc) override;
	private:
		PancystarEngine::EngineFailReason LoadPictureFromFile(PancyCommonTextureDesc &texture_desc);
		PancystarEngine::EngineFailReason BuildEmptyPicture(const PancyCommonTextureDesc &texture_desc);
		//将纹理图片采样至windows图片中
		PancystarEngine::EngineFailReason CaptureTextureDataToWindows(DirectX::ScratchImage *new_image);
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
			const unsigned int &loadFlags,
			const int32_t &subresources_num,
			PancyCommonTextureDesc &new_texture_desc
		);
		PancystarEngine::EngineFailReason UpdateTextureResource(std::vector<D3D12_SUBRESOURCE_DATA> &subresources,const PancyCommonTextureDesc &texture_desc);
		void RebuildTextureDataPath(const std::string &json_file_name, std::string &tex_data_file_name);
		void GetJsonFilePath(const std::string &json_file_name, std::string &file_path_out);
	};
	ResourceBlockGpu* GetTextureResourceData(VirtualResourcePointer &virtual_pointer, PancystarEngine::EngineFailReason &check_error);
	PancystarEngine::EngineFailReason LoadDDSTextureResource(
		const std::string &name_resource_in,
		VirtualResourcePointer &id_need
	);
	PancystarEngine::EngineFailReason BuildTextureResource(
		const std::string &name_resource_in,
		PancyCommonTextureDesc &resource_data,
		VirtualResourcePointer &id_need,
		bool if_allow_repeat
	);
	void InitTextureJsonReflect();
}