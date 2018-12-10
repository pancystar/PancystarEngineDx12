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
		D3D12_SHADER_RESOURCE_VIEW_DESC  tex_srv_desc = {};  //纹理访问格式
		D3D12_DEPTH_STENCIL_VIEW_DESC    tex_dsv_desc = {};  //深度模板缓冲格式
		//VirtualMemoryPointer             tex_data;      //纹理数据指针
		//todo：纹理拷贝
		bool                             if_copy_finish;       //纹理上传gpu是否完成
		SubMemoryPointer                 tex_data;             //纹理数据指针
		SubMemoryPointer                 update_tex_data;      //纹理上传数据指针
		PancyFenceIdGPU                  copy_broken_fence;
	public:
		PancyBasicTexture(
			std::string desc_file_in,
			bool if_gen_mipmap_in = false,
			bool if_force_srgb_in = false,
			int max_size_in = 0);
		PancystarEngine::EngineFailReason GetResource(SubMemoryPointer &resource)
		{
			if (!if_copy_finish) 
			{
				if (!ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)->CheckGpuBrokenFence(copy_broken_fence))
				{
					PancystarEngine::EngineFailReason error_message(E_FAIL,"The Texture haven't been copy finished to GPU, call WaitThreadPool to wait until it finished copy");
					PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Texture Resource", error_message);
					return error_message;
				}
				else 
				{
					if_copy_finish = true;
				}
			}
			resource = tex_data;
			return PancystarEngine::succeed;
		}
		inline D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc() 
		{
			return tex_srv_desc;
		}
		inline D3D12_DEPTH_STENCIL_VIEW_DESC   GetDSVDesc()
		{
			return tex_dsv_desc;
		}
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
			const unsigned int &loadFlags,
			const int32_t &subresources_num
		);
		PancystarEngine::EngineFailReason UpdateTextureResourceAndWait(std::vector<D3D12_SUBRESOURCE_DATA> &subresources);
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
	private:
		PancystarEngine::EngineFailReason BuildResource(const std::string &desc_file_in, PancyBasicVirtualResource** resource_out);
	};

	
}