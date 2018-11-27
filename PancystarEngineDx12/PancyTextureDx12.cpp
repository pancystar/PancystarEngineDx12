#include"PancyTextureDx12.h"
using namespace DirectX;
using namespace PancystarEngine;
uint32_t PancystarEngine::MyCountMips(uint32_t width, uint32_t height)
{
	if (width == 0 || height == 0)
		return 0;

	uint32_t count = 1;
	while (width > 1 || height > 1)
	{
		width >>= 1;
		height >>= 1;
		count++;
	}
	return count;
}
inline bool PancystarEngine::CheckIfDepthStencil(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_D16_UNORM:

#if defined(_XBOX_ONE) && defined(_TITLE)
	case DXGI_FORMAT_D16_UNORM_S8_UINT:
	case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
#endif
		return true;

	default:
		return false;
	}
}
inline void PancystarEngine::MyAdjustPlaneResource(
	_In_ DXGI_FORMAT fmt,
	_In_ size_t height,
	_In_ size_t slicePlane,
	_Inout_ D3D12_SUBRESOURCE_DATA& res)
{
	switch (fmt)
	{
	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:

#if defined(_XBOX_ONE) && defined(_TITLE)
	case DXGI_FORMAT_D16_UNORM_S8_UINT:
	case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
#endif
		if (!slicePlane)
		{
			// Plane 0
			res.SlicePitch = res.RowPitch * height;
		}
		else
		{
			// Plane 1
			res.pData = static_cast<const uint8_t*>(res.pData) + res.RowPitch * height;
			res.SlicePitch = res.RowPitch * ((height + 1) >> 1);
		}
		break;

	case DXGI_FORMAT_NV11:
		if (!slicePlane)
		{
			// Plane 0
			res.SlicePitch = res.RowPitch * height;
		}
		else
		{
			// Plane 1
			res.pData = static_cast<const uint8_t*>(res.pData) + res.RowPitch * height;
			res.RowPitch = (res.RowPitch >> 1);
			res.SlicePitch = res.RowPitch * height;
		}
		break;

	default:
		break;
	}
}
HRESULT PancystarEngine::MyFillInitData(_In_ size_t width,
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
	std::vector<D3D12_SUBRESOURCE_DATA>& initData)
{
	if (!bitData)
	{
		return E_POINTER;
	}

	skipMip = 0;
	twidth = 0;
	theight = 0;
	tdepth = 0;

	size_t NumBytes = 0;
	size_t RowBytes = 0;
	const uint8_t* pEndBits = bitData + bitSize;

	initData.clear();

	for (size_t p = 0; p < numberOfPlanes; ++p)
	{
		const uint8_t* pSrcBits = bitData;

		for (size_t j = 0; j < arraySize; j++)
		{
			size_t w = width;
			size_t h = height;
			size_t d = depth;
			for (size_t i = 0; i < mipCount; i++)
			{
				HRESULT hr = DirectX::LoaderHelpers::GetSurfaceInfo(w, h, format, &NumBytes, &RowBytes, nullptr);
				if (FAILED(hr))
					return hr;

				if (NumBytes > UINT32_MAX || RowBytes > UINT32_MAX)
					return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

				if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize))
				{
					if (!twidth)
					{
						twidth = w;
						theight = h;
						tdepth = d;
					}

					D3D12_SUBRESOURCE_DATA res =
					{
						pSrcBits,
						static_cast<LONG_PTR>(RowBytes),
						static_cast<LONG_PTR>(NumBytes)
					};

					MyAdjustPlaneResource(format, h, p, res);

					initData.emplace_back(res);
				}
				else if (!j)
				{
					// Count number of skipped mipmaps (first item only)
					++skipMip;
				}

				if (pSrcBits + (NumBytes*d) > pEndBits)
				{
					return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
				}

				pSrcBits += NumBytes * d;

				w = w >> 1;
				h = h >> 1;
				d = d >> 1;
				if (w == 0)
				{
					w = 1;
				}
				if (h == 0)
				{
					h = 1;
				}
				if (d == 0)
				{
					d = 1;
				}
			}
		}
	}

	return initData.empty() ? E_FAIL : S_OK;
}
PancyBasicTexture::PancyBasicTexture(std::string desc_file_in, bool if_gen_mipmap_in, bool if_force_srgb_in, int max_size_in) : PancystarEngine::PancyBasicVirtualResource(desc_file_in)
{
	if_force_srgb = if_force_srgb_in;
	if_gen_mipmap = if_gen_mipmap_in;
	max_size = max_size_in;
	if_cube_map = false;
}
std::string PancyBasicTexture::GetFileTile(const std::string &data_input)
{
	std::string out_pre;
	std::string out_final;
	for (int32_t i = static_cast<int32_t>(data_input.size() - 1); i >= 0; --i)
	{
		if (data_input[i] != '.')
		{
			out_pre += data_input[i];
		}
		else 
		{
			break;
		}
	}
	for (int32_t i = static_cast<int32_t>(out_pre.size() - 1); i >= 0; --i)
	{
		out_final += out_pre[i];
	}
	return out_final;
}
PancystarEngine::EngineFailReason PancyBasicTexture::LoadPictureFromFile(const std::string &picture_path_file)
{
	PancystarEngine::EngineFailReason check_error;
	PancystarEngine::PancyString file_name = picture_path_file;
	std::string file_type = GetFileTile(picture_path_file);
	if (file_type == "dds")
	{
		const DirectX::DDS_HEADER* header = nullptr;
		const uint8_t* bitData = nullptr;
		size_t bitSize = 0;
		std::unique_ptr<uint8_t[]> ddsData;
		//加载dds纹理数据
		HRESULT hr = DirectX::LoaderHelpers::LoadTextureDataFromFile(
			file_name.GetUnicodeString().c_str(),
			ddsData,
			&header,
			&bitData,
			&bitSize
		);
		if (FAILED(hr))
		{
			PancystarEngine::EngineFailReason error_message(hr, "load texture: " + picture_path_file + " error");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
			return error_message;
		}
		//检验纹理格式
		UINT width = header->width;
		UINT height = header->height;
		UINT depth = header->depth;
		D3D12_RESOURCE_DIMENSION resDim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
		UINT arraySize = 1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		bool isCubeMap = false;
		size_t mipCount = header->mipMapCount;
		if (0 == mipCount)
		{
			mipCount = 1;
		}
		if ((header->ddspf.flags & DDS_FOURCC) &&
			(MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
		{
			auto d3d10ext = reinterpret_cast<const DirectX::DDS_HEADER_DXT10*>(reinterpret_cast<const char*>(header) + sizeof(DirectX::DDS_HEADER));

			arraySize = d3d10ext->arraySize;
			if (arraySize == 0)
			{
				PancystarEngine::EngineFailReason error_message(ERROR_INVALID_DATA, "Texture: " + picture_path_file + " have a wrong size of array");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
				return error_message;
			}
			switch (d3d10ext->dxgiFormat)
			{
			case DXGI_FORMAT_AI44:
			case DXGI_FORMAT_IA44:
			case DXGI_FORMAT_P8:
			case DXGI_FORMAT_A8P8:
			{

				PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + " ERROR: DDSTextureLoader does not support video textures. Consider using DirectXTex instead.");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
				return error_message;
			}
			default:
				if (DirectX::LoaderHelpers::BitsPerPixel(d3d10ext->dxgiFormat) == 0)
				{
					PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Unknown DXGI format (" + std::to_string(static_cast<uint32_t>(d3d10ext->dxgiFormat)) + ")");
					PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
					return error_message;
				}
			}

			format = d3d10ext->dxgiFormat;

			switch (d3d10ext->resourceDimension)
			{
			case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
				// D3DX writes 1D textures with a fixed Height of 1
				if ((header->flags & DDS_HEIGHT) && height != 1)
				{
					PancystarEngine::EngineFailReason error_message(ERROR_INVALID_DATA, picture_path_file + "D3DX writes 1D textures with a fixed Height of 1");
					PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
					return error_message;
				}
				height = depth = 1;
				break;

			case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				if (d3d10ext->miscFlag & 0x4 /* RESOURCE_MISC_TEXTURECUBE */)
				{
					arraySize *= 6;
					isCubeMap = true;
				}
				depth = 1;
				break;

			case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
				if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
				{
					PancystarEngine::EngineFailReason error_message(ERROR_INVALID_DATA, picture_path_file + "ERROR: texture3d with volume texture");
					PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
					return error_message;
				}

				if (arraySize > 1)
				{
					PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Volume textures are not texture arrays");
					PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
					return error_message;
				}
				break;

			default:
				PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Unknown resource dimension: " + std::to_string(static_cast<uint32_t>(d3d10ext->resourceDimension)));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
				return error_message;
			}

			resDim = static_cast<D3D12_RESOURCE_DIMENSION>(d3d10ext->resourceDimension);
		}
		else
		{
			format = DirectX::LoaderHelpers::GetDXGIFormat(header->ddspf);

			if (format == DXGI_FORMAT_UNKNOWN)
			{
				PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: DDSTextureLoader does not support all legacy DDS formats. Consider using DirectXTex.");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
				return error_message;
			}

			if (header->flags & DDS_HEADER_FLAGS_VOLUME)
			{
				resDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			}
			else
			{
				if (header->caps2 & DDS_CUBEMAP)
				{
					// We require all six faces to be defined
					if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
					{
						PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: DirectX 12 does not support partial cubemaps");
						PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
						return error_message;
					}

					arraySize = 6;
					isCubeMap = true;
				}

				depth = 1;
				resDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

				// Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
			}

			assert(DirectX::LoaderHelpers::BitsPerPixel(format) != 0);
		}
		if (mipCount > D3D12_REQ_MIP_LEVELS)
		{
			PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Too many mipmap levels defined for DirectX 12: " + std::to_string(mipCount));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
			return error_message;
		}
		switch (resDim)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			if ((arraySize > D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
				(width > D3D12_REQ_TEXTURE1D_U_DIMENSION))
			{
				PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Resource dimensions too large for DirectX 12 (1D: array " + std::to_string(arraySize) + ", size " + std::to_string(width) + ")\n");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
				return error_message;
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (isCubeMap)
			{
				// This is the right bound because we set arraySize to (NumCubes*6) above
				if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
					(width > D3D12_REQ_TEXTURECUBE_DIMENSION) ||
					(height > D3D12_REQ_TEXTURECUBE_DIMENSION))
				{
					PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Resource dimensions too large for DirectX 12 (2D cubemap: " + std::to_string(arraySize) + ", size " + std::to_string(width) + "by " + std::to_string(height) + "%u)\n");
					PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
					return error_message;
				}
			}
			else if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
				(width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
				(height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION))
			{
				PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Resource dimensions too large for DirectX 12 (2D: " + std::to_string(arraySize) + ", size " + std::to_string(width) + "by " + std::to_string(height) + "%u)\n");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
				return error_message;
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			if ((arraySize > 1) ||
				(width > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
				(height > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
				(depth > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
			{
				PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Resource dimensions too large for DirectX 12 (3D: array " + std::to_string(arraySize) + ", size " + std::to_string(width) + " by " + std::to_string(height) + " by " + std::to_string(depth) + ")\n");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
				return error_message;
			}
			break;

		default:
			PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, picture_path_file + "ERROR: Unknown resource dimension (" + std::to_string(static_cast<uint32_t>(resDim)) + ")\n");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
			return error_message;
		}
		UINT numberOfPlanes = D3D12GetFormatPlaneCount(PancyDx12DeviceBasic::GetInstance()->GetD3dDevice().Get(), format);
		if (!numberOfPlanes)
		{
			PancystarEngine::EngineFailReason error_message(E_INVALIDARG, "load: " + picture_path_file + " error, device don't support specified format: " + std::to_string(format));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
			return error_message;
		}
		if ((numberOfPlanes > 1) && CheckIfDepthStencil(format))
		{
			// DirectX 12 uses planes for stencil, DirectX 11 does not
			PancystarEngine::EngineFailReason error_message(ERROR_NOT_SUPPORTED, "load: " + picture_path_file + " error, DirectX 12 uses planes for stencil, DirectX 11 does not");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
			return error_message;
		}
		if_cube_map = isCubeMap;
		//填充纹理数据
		size_t numberOfResources = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
			? 1 : arraySize;
		numberOfResources *= mipCount;
		numberOfResources *= numberOfPlanes;
		if (numberOfResources > D3D12_REQ_SUBRESOURCES)
		{
			PancystarEngine::EngineFailReason error_message(E_INVALIDARG, "load: " + picture_path_file + " error, texture3d can't afford number of resource: " + std::to_string(numberOfResources));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
			return error_message;
		}
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		subresources.reserve(numberOfResources);
		size_t skipMip = 0;
		size_t twidth = 0;
		size_t theight = 0;
		size_t tdepth = 0;
		hr = MyFillInitData(width, height, depth, mipCount, arraySize,
			numberOfPlanes, format,
			max_size, bitSize, bitData,
			twidth, theight, tdepth, skipMip, subresources);
		DirectX::DDS_LOADER_FLAGS loadFlags;
		if (if_gen_mipmap)
		{
			loadFlags = DirectX::DDS_LOADER_MIP_AUTOGEN;
		}
		else
		{
			loadFlags = DirectX::DDS_LOADER_DEFAULT;
		}
		if (if_force_srgb)
		{
			loadFlags = static_cast<DDS_LOADER_FLAGS>(loadFlags | DirectX::DDS_LOADER_FORCE_SRGB);
		}
		if (SUCCEEDED(hr))
		{
			size_t reservedMips = mipCount;
			if (loadFlags & (DirectX::DDS_LOADER_MIP_AUTOGEN | DirectX::DDS_LOADER_MIP_RESERVE))
			{
				reservedMips = std::min<size_t>(D3D12_REQ_MIP_LEVELS, MyCountMips(width, height));
			}

			check_error = BuildTextureResource(resDim, twidth, theight, tdepth, reservedMips - skipMip, arraySize,
				format, D3D12_RESOURCE_FLAG_NONE, loadFlags);
			//修改纹理大小限制重新加载
			if (!check_error.CheckIfSucceed())
			{
				if (!max_size && (mipCount > 1)) 
				{
					subresources.clear();

					max_size = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
						? D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION
						: D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;

					hr = MyFillInitData(width, height, depth, mipCount, arraySize,
						numberOfPlanes, format,
						max_size, bitSize, bitData,
						twidth, theight, tdepth, skipMip, subresources);
					if (SUCCEEDED(hr))
					{
						check_error = BuildTextureResource(resDim, twidth, theight, tdepth, mipCount - skipMip, arraySize,
							format, D3D12_RESOURCE_FLAG_NONE, loadFlags);
						if (!check_error.CheckIfSucceed())
						{
							PancystarEngine::EngineFailReason error_message(E_INVALIDARG, "load: " + picture_path_file + "error, second try create resource");
							PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
							return error_message;
						}
					}
					else
					{
						PancystarEngine::EngineFailReason error_message(E_INVALIDARG, "load: " + picture_path_file + "error, second try fillInitData failed");
						PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
						return error_message;
					}
				}
				else 
				{
					return check_error;
				}
			}
			//todo:拷贝纹理
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_INVALIDARG, "load: " + picture_path_file + "error, first try fillInitData failed");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
			return error_message;
		}
	}
	
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicTexture::UpdateTextureResourceAndWait(const std::vector<D3D12_SUBRESOURCE_DATA> &subresources)
{
	const D3D12_SUBRESOURCE_DATA *subres = &subresources[0];
	UINT subres_size = static_cast<UINT>(subresources.size());
	int64_t res_size;
	ComPtr<ID3D12Resource> tex_data_res = SubresourceControl::GetInstance()->GetResourceData(tex_data, res_size)->GetResource();
	PancyRenderCommandList *copy_render_list;
	uint32_t copy_render_list_ID;

	std::string bufferblock_file_name = "json\\resource_view\\Subtex_";
	std::string heap_name = "json\\resource_heap\\tex_";
	heap_name += std::to_string(desc.Width);
	heap_name += "_";
	heap_name += std::to_string(desc.Height);

	bufferblock_file_name += std::to_string(desc.Width);
	bufferblock_file_name += "_";
	bufferblock_file_name += std::to_string(desc.Height);

	//auto copy_contex = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetEmptyRenderlist(NULL, D3D12_COMMAND_LIST_TYPE_DIRECT, &copy_render_list, copy_render_list_ID);
	//auto update_thread = pancy;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicTexture::BuildTextureResource(
	const D3D12_RESOURCE_DIMENSION &resDim,
	const size_t &width,
	const size_t &height,
	const size_t &depth,
	const size_t &mipCount,
	const size_t &arraySize,
	DXGI_FORMAT &format,
	const D3D12_RESOURCE_FLAGS &resFlags,
	const unsigned int &loadFlags
)
{
	PancystarEngine::EngineFailReason check_error;

	if (loadFlags & DirectX::DDS_LOADER_FORCE_SRGB)
	{
		format = DirectX::LoaderHelpers::MakeSRGB(format);
	}

	D3D12_RESOURCE_DESC desc = {};
	desc.Width = static_cast<UINT>(width);
	desc.Height = static_cast<UINT>(height);
	desc.MipLevels = static_cast<UINT16>(mipCount);
	desc.DepthOrArraySize = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? static_cast<UINT16>(depth) : static_cast<UINT16>(arraySize);
	desc.Format = format;
	desc.Flags = resFlags;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Dimension = resDim;
	CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	//获取纹理名称
	std::string bufferblock_file_name = "json\\resource_view\\Subtex_";
	std::string heap_name = "json\\resource_heap\\tex_";
	heap_name += std::to_string(desc.Width);
	heap_name += "_";
	heap_name += std::to_string(desc.Height);

	bufferblock_file_name += std::to_string(desc.Width);
	bufferblock_file_name += "_";
	bufferblock_file_name += std::to_string(desc.Height);
	//非压缩纹理
	if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM || desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM || desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)
	{
		heap_name += "_4_singlemip";
		bufferblock_file_name += "_4_singlemip";
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(E_INVALIDARG, " the format of texture: " + resource_name + " not surport: " + std::to_string(desc.Format));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
		return error_message;
	}
	heap_name += ".json";
	bufferblock_file_name += ".json";
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(heap_name))
	{
		//更新格式文件
		Json::Value json_data_out;
		UINT texture_size = desc.Width * desc.Height * 4;
		UINT resource_block_num = 16777216 / texture_size;
		if (resource_block_num < 1)
		{
			resource_block_num = 1;
		}
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", texture_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", "D3D12_HEAP_TYPE_DEFAULT");
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_DENY_BUFFERS");
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES");
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, heap_name);
		//将文件标记为已经创建
		FileBuildRepeatCheck::GetInstance()->AddFileName(heap_name);
	}
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(bufferblock_file_name))
	{
		UINT texture_size = desc.Width * desc.Height * 4;
		//更新格式文件
		Json::Value json_data_resourceview;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", heap_name);
		Json::Value json_data_res_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", PancyJsonTool::GetInstance()->GetEnumName(typeid(desc.Dimension).name(), desc.Dimension));
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", desc.Width);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Height", desc.Height);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "DepthOrArraySize", desc.DepthOrArraySize);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "MipLevels", desc.MipLevels);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Format", PancyJsonTool::GetInstance()->GetEnumName(typeid(desc.Format).name(), desc.Format));
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Layout", PancyJsonTool::GetInstance()->GetEnumName(typeid(desc.Layout).name(), desc.Layout));
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Flags", PancyJsonTool::GetInstance()->GetEnumName(typeid(desc.Flags).name(), desc.Flags));
		Json::Value json_data_sample_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Count", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Quality", 0);
		//递归回调
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "SampleDesc", json_data_sample_desc);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_DESC", json_data_res_desc);
		//继续填充主干
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", "D3D12_RESOURCE_STATE_COPY_DEST");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", texture_size);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, bufferblock_file_name);
		FileBuildRepeatCheck::GetInstance()->AddFileName(bufferblock_file_name);
	}
	check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(bufferblock_file_name, tex_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
bool PancyBasicTexture::CheckIfJson(const std::string &path_name)
{
	if (path_name.substr(path_name.size() - 4, 4) == "json")
	{
		return true;
	}
	return false;
}
PancystarEngine::EngineFailReason PancyBasicTexture::BuildEmptyPicture(const std::string &picture_desc_file)
{
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicTexture::InitResource(const std::string &resource_desc_file)
{
	if (!CheckIfJson(resource_desc_file)) 
	{
		return LoadPictureFromFile(resource_desc_file);
	}
	else 
	{
		return BuildEmptyPicture(resource_desc_file);
	}
}
//纹理管理器
PancyTextureControl::PancyTextureControl(const std::string &resource_type_name_in) :PancystarEngine::PancyBasicResourceControl(resource_type_name_in)
{
}
PancystarEngine::EngineFailReason PancyTextureControl::BuildResource(const std::string &desc_file_in, PancyBasicVirtualResource** resource_out)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_json_value rec_value;
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(desc_file_in, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyJsonTool::GetInstance()->GetJsonData(desc_file_in, root_value, "IfFromFile", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (rec_value.int_value == 1)
	{
		std::string tex_file_name;
		bool if_auto_mipmap;
		bool if_force_srgb;
		int32_t max_size;
		check_error = PancyJsonTool::GetInstance()->GetJsonData(desc_file_in, root_value, "FileName", pancy_json_data_type::json_data_string, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		tex_file_name = rec_value.string_value;
		//是否自动创建mipmap
		check_error = PancyJsonTool::GetInstance()->GetJsonData(desc_file_in, root_value, "IfAutoBuildMipMap", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if (rec_value.int_value == 1)
		{
			if_auto_mipmap = true;
		}
		else
		{
			if_auto_mipmap = false;
		}
		//是否强制转换为srgb
		check_error = PancyJsonTool::GetInstance()->GetJsonData(desc_file_in, root_value, "IfForceSrgb", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if (rec_value.int_value == 1)
		{
			if_force_srgb = true;
		}
		else
		{
			if_force_srgb = false;
		}
		//最大内存大小
		check_error = PancyJsonTool::GetInstance()->GetJsonData(desc_file_in, root_value, "MaxSize", pancy_json_data_type::json_data_int, rec_value);
		max_size = rec_value.int_value;
		*resource_out = new PancyBasicTexture(tex_file_name, if_auto_mipmap, if_force_srgb, max_size);
	}
	else
	{
		*resource_out = new PancyBasicTexture(desc_file_in);
	}
	return PancystarEngine::succeed;
}