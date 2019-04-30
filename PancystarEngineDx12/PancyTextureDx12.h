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
	class PancyBasicTexture : public PancystarEngine::PancyBasicVirtualResource
	{
		D3D12_RESOURCE_DESC              desc;//纹理格式数据
		bool                             if_cube_map;   //是否是cubemap(仅dds有效)
		bool                             if_gen_mipmap; //是否为无mipmap的纹理创建mipmap
		bool                             if_force_srgb; //是否强制转换为线性空间纹理
		int                              max_size;      //纹理最大大小
		bool                             if_from_file;  //是否是从文件中加载
		D3D12_SHADER_RESOURCE_VIEW_DESC  tex_srv_desc = {};  //纹理访问格式
		//D3D12_DEPTH_STENCIL_VIEW_DESC    tex_dsv_desc = {};  //深度模板缓冲格式
		//D3D12_RENDER_TARGET_VIEW_DESC    tex_rtv_desc = {};  //渲染目标格式
		SubMemoryPointer                 tex_data;             //纹理数据指针
		pancy_object_id                  upload_buffer_id;     //上传缓冲区ID号
		PancyFenceIdGPU                  copy_broken_fence;
	public:
		PancyBasicTexture(
			const std::string &resource_name,
			const Json::Value &root_value
		);
		~PancyBasicTexture();
		PancystarEngine::EngineFailReason GetResource(SubMemoryPointer &resource)
		{
			auto resource_state = GetResourceState();
			if (resource_state == ResourceStateType::resource_state_not_init)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "The Texture resource init failed");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Texture Resource", error_message);
				return error_message;
			}
			else if (resource_state == ResourceStateType::resource_state_load_CPU_memory_finish)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "The Texture haven't been copy finished to GPU, call WaitThreadPool to wait until it finished copy");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Texture Resource", error_message);
				return error_message;
			}
			resource = tex_data;
			return PancystarEngine::succeed;
		}
		PancystarEngine::EngineFailReason SaveTextureToFile(
			ID3D11Device* pDevice,
			const std::string &file_name,
			bool if_automip = false,
			bool if_compress = false
		);
		inline D3D12_RESOURCE_DESC GetResourceDesc() 
		{
			return desc;
		}
		inline  PancystarEngine::EngineFailReason GetSRVDesc(D3D12_SHADER_RESOURCE_VIEW_DESC &SRV_Desc_out)
		{
			/*
			if (!if_from_file) 
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL,"the controler could only build SRV from the texture from file,other texture SRV must build by custom");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get shader resource view from texture",error_message);
				return error_message;
			}
			*/
			SRV_Desc_out = tex_srv_desc;
			return PancystarEngine::succeed;
		}
		/*
		inline D3D12_DEPTH_STENCIL_VIEW_DESC   GetDSVDesc()
		{
			return tex_dsv_desc;
		}
		inline D3D12_RENDER_TARGET_VIEW_DESC   GetRTVDesc()
		{
			return tex_rtv_desc;
		}
		*/
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name, ResourceStateType &now_res_state);
		//检测当前的资源是否已经被载入GPU
		void CheckIfResourceLoadToGpu(ResourceStateType &now_res_state);
		PancystarEngine::EngineFailReason LoadPictureFromFile(const std::string &picture_path_file, ResourceStateType &now_res_state);
		//PancystarEngine::EngineFailReason BuildEmptyPicture(const std::string &picture_desc_file);
		PancystarEngine::EngineFailReason BuildEmptyPicture(const Json::Value &root_value);
		
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
			const int32_t &subresources_num
		);
		PancystarEngine::EngineFailReason UpdateTextureResource(std::vector<D3D12_SUBRESOURCE_DATA> &subresources);
		bool CheckIfJson(const std::string &path_name);
		inline bool CheckIfPow2(int32_t input) 
		{
			if (input == 32 || input == 64 || input == 128 || input == 256 || input == 512 || input == 1024 || input == 2048 || input == 4096) 
			{
				return true;
			}
			return false;
		}
		void RebuildTextureDataPath(const std::string &json_file_name, std::string &tex_data_file_name);
		void GetJsonFilePath(const std::string &json_file_name, std::string &file_path_out);
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
		inline PancystarEngine::EngineFailReason GetTexResource(const pancy_object_id &tex_id, SubMemoryPointer &res_pointer)
		{
			PancyBasicTexture *tex_data = dynamic_cast<PancyBasicTexture*>(GetResource(tex_id));
			if (tex_data == NULL) 
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL,"Could not find the Texture ID: "+std::to_string(tex_id));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Texture Resource", error_message);
				return error_message;
			}
			return tex_data->GetResource(res_pointer);
		};
		inline PancystarEngine::EngineFailReason GetTexDesc(const pancy_object_id &tex_id, D3D12_RESOURCE_DESC &res_desc)
		{
			PancyBasicTexture *tex_data = dynamic_cast<PancyBasicTexture*>(GetResource(tex_id));
			if (tex_data == NULL)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find the Texture ID: " + std::to_string(tex_id));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Texture SRV desc", error_message);
				return error_message;
			}
			res_desc = tex_data->GetResourceDesc();
			return PancystarEngine::succeed;
		}
		inline PancystarEngine::EngineFailReason GetSRVDesc(const pancy_object_id &tex_id, D3D12_SHADER_RESOURCE_VIEW_DESC &SRV_desc)
		{
			PancystarEngine::EngineFailReason check_error;
			PancyBasicTexture *tex_data = dynamic_cast<PancyBasicTexture*>(GetResource(tex_id));
			if (tex_data == NULL)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find the Texture ID: " + std::to_string(tex_id));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Texture SRV desc", error_message);
				return error_message;
			}
			check_error = tex_data->GetSRVDesc(SRV_desc);
			if (!check_error.CheckIfSucceed()) 
			{
				return check_error;
			}
			return PancystarEngine::succeed;
		}
		/*
		inline PancystarEngine::EngineFailReason GetSRVDesc(const pancy_object_id &tex_id, D3D12_SHADER_RESOURCE_VIEW_DESC &SRV_desc)
		{ 
			PancyBasicTexture *tex_data = dynamic_cast<PancyBasicTexture*>(GetResource(tex_id));
			if (tex_data == NULL)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find the Texture ID: " + std::to_string(tex_id));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Texture SRV desc", error_message);
				return error_message;
			}
			SRV_desc = tex_data->GetSRVDesc();
			return PancystarEngine::succeed;
		}
		inline PancystarEngine::EngineFailReason GetDSVDesc(const pancy_object_id &tex_id, D3D12_DEPTH_STENCIL_VIEW_DESC &DSV_desc)
		{
			PancyBasicTexture *tex_data = dynamic_cast<PancyBasicTexture*>(GetResource(tex_id));
			if (tex_data == NULL)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find the Texture ID: " + std::to_string(tex_id));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Texture DSV desc", error_message);
				return error_message;
			}
			DSV_desc = tex_data->GetDSVDesc();
			return PancystarEngine::succeed;
		}
		inline PancystarEngine::EngineFailReason GetRTVDesc(const pancy_object_id &tex_id, D3D12_RENDER_TARGET_VIEW_DESC &RTV_desc)
		{
			PancyBasicTexture *tex_data = dynamic_cast<PancyBasicTexture*>(GetResource(tex_id));
			if (tex_data == NULL)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find the Texture ID: " + std::to_string(tex_id));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Texture RTV desc", error_message);
				return error_message;
			}
			RTV_desc = tex_data->GetRTVDesc();
			return PancystarEngine::succeed;
		}
		*/
		//根据指定的纹理资源类型，创建一套确定纹理资源格式的json文件(heap,subresource)
		PancystarEngine::EngineFailReason BuildTextureTypeJson(
			const D3D12_RESOURCE_DESC &subresource_desc,
			int32_t resource_num,
			D3D12_HEAP_TYPE heap_type,
			const std::vector<D3D12_HEAP_FLAGS> &heap_flags,
			D3D12_RESOURCE_STATES res_state,
			std::string &subresource_desc_name
		);
		PancystarEngine::EngineFailReason SaveTextureToFile(
			ID3D11Device* pDevice,
			pancy_object_id texture_id,
			const std::string &file_name,
			bool if_automip = false,
			bool if_compress = false
		);
	private:
		PancystarEngine::EngineFailReason BuildResource(
			const Json::Value &root_value,
			const std::string &name_resource_in,
			PancyBasicVirtualResource** resource_out
		);
	};
	
}