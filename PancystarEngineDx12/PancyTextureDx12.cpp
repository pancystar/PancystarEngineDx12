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
PancyBasicTexture::PancyBasicTexture(std::string desc_file_in) : PancystarEngine::PancyBasicVirtualResource(desc_file_in)
{
	if_force_srgb = false;
	if_gen_mipmap = false;
	max_size = 0;
	if_cube_map = false;
	if_copy_finish = false;
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
		//获取创建格式
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
				format, D3D12_RESOURCE_FLAG_NONE, loadFlags, subresources.size());
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
							format, D3D12_RESOURCE_FLAG_NONE, loadFlags, subresources.size());
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
			UpdateTextureResourceAndWait(subresources);
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_INVALIDARG, "load: " + picture_path_file + "error, first try fillInitData failed");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
			return error_message;
		}
	}
	else 
	{
		PancystarEngine::EngineFailReason error_message(ERROR_INVALID_DATA, "un supported texture type:" + picture_path_file);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Texture From Picture", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicTexture::UpdateTextureResourceAndWait(std::vector<D3D12_SUBRESOURCE_DATA> &subresources)
{
	PancystarEngine::EngineFailReason check_error;
	D3D12_SUBRESOURCE_DATA *subres = &subresources[0];
	UINT subres_size = static_cast<UINT>(subresources.size());
	int64_t res_size;
	auto tex_data_res = SubresourceControl::GetInstance()->GetResourceData(tex_data, res_size);
	auto copy_data_res = SubresourceControl::GetInstance()->GetResourceData(update_tex_data, res_size);
	PancyRenderCommandList *copy_render_list;
	PancyThreadIdGPU copy_render_list_ID;
	//开始拷贝

	//获取拷贝所用的commandlist
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//先将数据从内存拷贝到上传缓冲区
	UINT64 RequiredSize = 0;
	UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * subres_size;
	if (MemToAlloc > SIZE_MAX)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "subresource size is bigger than max_size");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("copy CPU resource to update texture", error_message);
		return error_message;
	}
	void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
	if (pMem == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "alloc heap for write resource failed");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("copy CPU resource to update texture", error_message);
		return error_message;
	}
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
	UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + subres_size);
	UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + subres_size);
	D3D12_RESOURCE_DESC Desc = tex_data_res->GetResource()->GetDesc();
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&Desc, 0, subres_size, 0, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
	check_error = copy_data_res->WriteFromCpuToBuffer(0, subresources, pLayouts, pRowSizesInBytes, pNumRows);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//再将数据从上传缓冲区拷贝至显存
	for (UINT i = 0; i < subres_size; ++i)
	{
		CD3DX12_TEXTURE_COPY_LOCATION Dst(tex_data_res->GetResource().Get(), i + 0);
		CD3DX12_TEXTURE_COPY_LOCATION Src(copy_data_res->GetResource().Get(), pLayouts[i]);
		copy_render_list->GetCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
	}
	copy_render_list->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(tex_data_res->GetResource().Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	copy_render_list->UnlockPrepare();
	ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &copy_render_list_ID);
	HeapFree(GetProcessHeap(), 0, pMem);
	//插眼
	ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(copy_broken_fence);
	//等待
	ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(copy_broken_fence);
	//释放资源
	SubresourceControl::GetInstance()->FreeSubResource(update_tex_data);
	if_copy_finish = true;
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
	const unsigned int &loadFlags,
	const int32_t &subresources_num
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
	uint64_t subresources_size;
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&desc, 0, subresources_num, 0, nullptr, nullptr, nullptr, &subresources_size);
	if (subresources_size % 65536 != 0)
	{
		subresources_size = (subresources_size + 65536) & ~65535;
	}
	CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	tex_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	tex_srv_desc.Format = desc.Format;
	if (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
	{
		tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
	}
	else if (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		if (!if_cube_map)
		{
			tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		}
		else
		{
			tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		}
	}
	else if (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
	{
		tex_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	}
	tex_srv_desc.Texture2D.MipLevels = desc.MipLevels;
	//获取纹理名称
	std::string bufferblock_file_name = "json\\resource_view\\Subtex_";
	std::string heap_name = "json\\resource_heap\\tex_";

	heap_name += std::to_string(desc.Width);
	heap_name += "_";
	heap_name += std::to_string(desc.Height);

	bufferblock_file_name += std::to_string(desc.Width);
	bufferblock_file_name += "_";
	bufferblock_file_name += std::to_string(desc.Height);

	string dxgi_name = PancyJsonTool::GetInstance()->GetEnumName(typeid(tex_srv_desc.Format).name(), tex_srv_desc.Format);
	string sub_dxgi_name = dxgi_name.substr(12, dxgi_name.size() - 12);
	heap_name += "_" + sub_dxgi_name;
	bufferblock_file_name += "_" + sub_dxgi_name;
	heap_name += "_" + std::to_string(desc.MipLevels) + "mip";
	bufferblock_file_name += "_" + std::to_string(desc.MipLevels) + "mip";
	if (if_cube_map)
	{
		heap_name += "_cube";
		bufferblock_file_name += "_cube";
	}
	/*
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
	*/
	heap_name += ".json";
	bufferblock_file_name += ".json";
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(heap_name))
	{
		//更新格式文件
		Json::Value json_data_out;
		UINT resource_block_num = (TextureHeapAliaze * 4) / subresources_size;
		UINT copy_resource_block_num = (TextureHeapAliaze) / subresources_size;
		if (resource_block_num < 1)
		{
			resource_block_num = 1;
		}
		if (resource_block_num > 32)
		{
			resource_block_num = 32;
		}
		if (copy_resource_block_num < 1)
		{
			copy_resource_block_num = 1;
		}
		if (copy_resource_block_num > 16)
		{
			copy_resource_block_num = 16;
		}
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", subresources_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", "D3D12_HEAP_TYPE_DEFAULT");
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_DENY_BUFFERS");
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES");
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, heap_name);
		//将文件标记为已经创建
		FileBuildRepeatCheck::GetInstance()->AddFileName(heap_name);
	}
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(bufferblock_file_name))
	{
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
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", subresources_size);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, bufferblock_file_name);
		FileBuildRepeatCheck::GetInstance()->AddFileName(bufferblock_file_name);
		/*
		//填充拷贝资源
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", copy_heap_name);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", "D3D12_RESOURCE_STATE_GENERIC_READ");
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, copy_bufferblock_file_name);
		FileBuildRepeatCheck::GetInstance()->AddFileName(copy_bufferblock_file_name);
		*/
	}
	check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(bufferblock_file_name, tex_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//获取纹理拷贝缓冲区的显存大小
	int64_t size_in;
	UINT64 uploadSize = subresources_size;
	//创建纹理拷贝缓冲区
	std::string copy_heap_name = "json\\resource_heap\\DynamicBuffer" + std::to_string(uploadSize) + ".json";
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(copy_heap_name))
	{
		//更新格式文件
		Json::Value json_data_out;
		UINT resource_block_num = (BufferHeapAliaze * 5) / uploadSize;
		if (resource_block_num < 1)
		{
			resource_block_num = 1;
		}
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", uploadSize);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", "D3D12_HEAP_TYPE_UPLOAD");
		PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS");
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, copy_heap_name);
		//将文件标记为已经创建
		FileBuildRepeatCheck::GetInstance()->AddFileName(copy_heap_name);
	}
	std::string copy_bufferblock_file_name = "json\\resource_view\\DynamicBufferSub" + std::to_string(uploadSize) + ".json";
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(copy_bufferblock_file_name))
	{
		//更新格式文件
		Json::Value json_data_resourceview;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", copy_heap_name);
		Json::Value json_data_res_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", "D3D12_RESOURCE_DIMENSION_BUFFER");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", uploadSize);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Height", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "DepthOrArraySize", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "MipLevels", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Format", "DXGI_FORMAT_UNKNOWN");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Layout", "D3D12_TEXTURE_LAYOUT_ROW_MAJOR");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Flags", "D3D12_RESOURCE_FLAG_NONE");
		Json::Value json_data_sample_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Count", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Quality", 0);
		//递归回调
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "SampleDesc", json_data_sample_desc);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_DESC", json_data_res_desc);
		//继续填充主干
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", "D3D12_RESOURCE_STATE_GENERIC_READ");
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", uploadSize);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, copy_bufferblock_file_name);
		FileBuildRepeatCheck::GetInstance()->AddFileName(copy_bufferblock_file_name);
	}
	check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(copy_bufferblock_file_name, update_tex_data);
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
	PancystarEngine::EngineFailReason check_error;
	pancy_json_value rec_value;
	Json::Value root_value;
	Json::Value sub_res_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(picture_desc_file, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//加载次级资源的格式文件
	tex_dsv_desc.Texture2D.MipSlice = 0;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(picture_desc_file, root_value, "SubResourceFile", pancy_json_data_type::json_data_string, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//根据纹理资源格式创建纹理资源
	std::string subresource_file_name = rec_value.string_value;
	check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(subresource_file_name, tex_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//加载DSV资料
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(subresource_file_name, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	Json::Value resource_desc = root_value.get("D3D12_RESOURCE_DESC", Json::Value::null);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(picture_desc_file, resource_desc, "Format", pancy_json_data_type::json_data_enum, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	tex_dsv_desc.Format = static_cast<DXGI_FORMAT>(rec_value.int_value);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(picture_desc_file, resource_desc, "Dimension", pancy_json_data_type::json_data_enum, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	tex_dsv_desc.ViewDimension = static_cast<D3D12_DSV_DIMENSION>(rec_value.int_value);

	tex_rtv_desc.Format = tex_dsv_desc.Format;
	tex_rtv_desc.Texture2D.MipSlice = 0;

	tex_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	tex_srv_desc.Format = tex_dsv_desc.Format;
	tex_srv_desc.Texture2D.MipLevels = 1;
	//空纹理不需要拷贝操作
	if_copy_finish = true;
	return PancystarEngine::succeed;
}
void PancyBasicTexture::GetJsonFilePath(const std::string &json_file_name, std::string &file_path_out)
{
	file_path_out = "";
	int32_t copy_size = json_file_name.size();
	for (int i = json_file_name.size() - 1; i >= 0; --i)
	{
		if (json_file_name[i] != '\\' && json_file_name[i] != '/')
		{
			copy_size -= 1;
		}
		else
		{
			break;
		}
	}
	file_path_out = json_file_name.substr(0, copy_size);
}
void PancyBasicTexture::RebuildTextureDataPath(const std::string &json_file_name, std::string &tex_data_file_name)
{
	bool if_change_path = true;
	//先检查纹理数据路径是否为相对路径
	for (int i = 0; i < tex_data_file_name.size(); ++i)
	{
		if (tex_data_file_name[i] == '\\')
		{
			//路径是绝对路径，不做修改
			if_change_path = false;
			break;
		}
	}
	if (if_change_path)
	{
		//路径是相对路径，需要手动添加绝对路径位置
		string path_file;
		GetJsonFilePath(json_file_name, path_file);
		tex_data_file_name = path_file + tex_data_file_name;
	}
}
PancystarEngine::EngineFailReason PancyBasicTexture::InitResource(const std::string &resource_desc_file)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_json_value rec_value;
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(resource_desc_file, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "IfFromFile", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (rec_value.int_value == 1)
	{
		std::string tex_file_name;
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "FileName", pancy_json_data_type::json_data_string, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		tex_file_name = rec_value.string_value;
		//根据路径格式决定是否修改为绝对路径
		RebuildTextureDataPath(resource_desc_file, tex_file_name);
		//是否自动创建mipmap
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "IfAutoBuildMipMap", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if (rec_value.int_value == 1)
		{
			if_gen_mipmap = true;
		}
		else
		{
			if_gen_mipmap = false;
		}
		//是否强制转换为srgb
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "IfForceSrgb", pancy_json_data_type::json_data_int, rec_value);
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
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "MaxSize", pancy_json_data_type::json_data_int, rec_value);
		max_size = rec_value.int_value;
		return LoadPictureFromFile(tex_file_name);
	}
	else
	{
		return BuildEmptyPicture(resource_desc_file);
	}
}
PancystarEngine::EngineFailReason PancyBasicTexture::SaveTextureToFile(
	ID3D11Device* pDevice,
	const std::string &file_name,
	bool if_automip,
	bool if_compress
)
{
	HRESULT hr;
	int64_t per_memory_size;
	auto res_data = SubresourceControl::GetInstance()->GetResourceData(tex_data, per_memory_size);
	DirectX::ScratchImage *new_image = NULL, *mipmap_image = NULL, *compress_image = NULL;
	bool if_mip_gen = false, if_compress_gen = false;
	new_image = new DirectX::ScratchImage();
	//将纹理数据拍摄到图片中
	DirectX::CaptureTexture(
		PancyDx12DeviceBasic::GetInstance()->GetCommandQueueDirect().Get(),
		res_data->GetResource().Get(),
		if_cube_map,
		*new_image,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	auto texture_desc = res_data->GetResource()->GetDesc();
	//为纹理创建mipmap
	if (if_automip && texture_desc.MipLevels == 1)
	{
		int32_t mipmap_level = MyCountMips(texture_desc.Width, texture_desc.Height);
		mipmap_image = new DirectX::ScratchImage();
		DirectX::GenerateMipMaps(*new_image->GetImages(), TEX_FILTER_DEFAULT | TEX_FILTER_FORCE_NON_WIC, mipmap_level, *mipmap_image);
		if_mip_gen = true;
	}
	else
	{
		mipmap_image = new_image;
	}
	//为纹理创建压缩格式(等microsoft更新/手动使用dx11版本)
	if (if_compress)
	{
		compress_image = new DirectX::ScratchImage();
		DXGI_FORMAT compress_type;
		if (texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || texture_desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB || texture_desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB)
		{
			compress_type = DXGI_FORMAT_BC7_UNORM_SRGB;
		}
		else if (texture_desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM || texture_desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM || texture_desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM)
		{
			compress_type = DXGI_FORMAT_BC7_UNORM;
		}
		else 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "compress format could not recognize: " + std::to_string(texture_desc.Format));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Save texture file:" + file_name, error_message);
			return error_message;
		}
		hr = DirectX::Compress(pDevice,
			mipmap_image->GetImages(),
			mipmap_image->GetImageCount(),
			mipmap_image->GetMetadata(),
			compress_type,
			TEXTURE_COMPRESS_BC7,
			1,
			*compress_image
		);
		if_compress_gen = true;
	}
	else
	{
		compress_image = mipmap_image;
	}
	PancystarEngine::PancyString file_name_sacii(file_name);
	DirectX::SaveToDDSFile(compress_image->GetImages(), compress_image->GetImageCount(), compress_image->GetMetadata(), DDS_FLAGS_NONE, file_name_sacii.GetUnicodeString().c_str());
	delete new_image;
	if (if_mip_gen)
	{
		delete mipmap_image;
	}
	if (if_compress_gen)
	{
		delete compress_image;
	}
	return PancystarEngine::succeed;
}
//纹理管理器
PancyTextureControl::PancyTextureControl(const std::string &resource_type_name_in) :PancystarEngine::PancyBasicResourceControl(resource_type_name_in)
{
}
PancystarEngine::EngineFailReason PancyTextureControl::BuildResource(const std::string &desc_file_in, PancyBasicVirtualResource** resource_out)
{
	*resource_out = new PancyBasicTexture(desc_file_in);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyTextureControl::SaveTextureToFile(
	ID3D11Device* pDevice,
	pancy_object_id texture_id,
	const std::string &file_name,
	bool if_automip,
	bool if_compress
)
{
	auto tex_res_data = GetResource(texture_id);
	if (tex_res_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the texture resource: " + std::to_string(texture_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Save texture file:" + file_name, error_message);
		return error_message;
	}
	PancyBasicTexture *tex_data = dynamic_cast<PancyBasicTexture*>(tex_res_data);
	PancystarEngine::EngineFailReason check_error = tex_data->SaveTextureToFile(pDevice,file_name, if_automip, if_compress);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyTextureControl::BuildTextureTypeJson(
	const D3D12_RESOURCE_DESC &subresource_desc,
	int32_t resource_num,
	D3D12_HEAP_TYPE heap_type,
	const std::vector<D3D12_HEAP_FLAGS> &heap_flags,
	D3D12_RESOURCE_STATES res_state,
	std::string &subresource_desc_name)
{
	uint64_t subresources_size;
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&subresource_desc, 0, resource_num, 0, nullptr, nullptr, nullptr, &subresources_size);
	if (subresources_size % 65536 != 0)
	{
		subresources_size = (subresources_size + 65536) & ~65535;
	}
	//计算存储堆和存储单元的名称
	std::string bufferblock_file_name = "json\\resource_view\\Subtex_";
	std::string heap_name = "json\\resource_heap\\tex_";

	heap_name += std::to_string(subresource_desc.Width);
	heap_name += "_";
	heap_name += std::to_string(subresource_desc.Height);

	bufferblock_file_name += std::to_string(subresource_desc.Width);
	bufferblock_file_name += "_";
	bufferblock_file_name += std::to_string(subresource_desc.Height);

	string dxgi_name = PancyJsonTool::GetInstance()->GetEnumName(typeid(subresource_desc.Format).name(), subresource_desc.Format);
	string sub_dxgi_name = dxgi_name.substr(12, dxgi_name.size() - 12);
	heap_name += "_" + sub_dxgi_name;
	bufferblock_file_name += "_" + sub_dxgi_name;
	heap_name += "_" + std::to_string(subresource_desc.MipLevels) + "mip";
	bufferblock_file_name += "_" + std::to_string(subresource_desc.MipLevels) + "mip";
	heap_name += ".json";
	bufferblock_file_name += ".json";
	//检查并创建资源存储堆
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(heap_name))
	{
		//文件未创建，创建文件
		Json::Value json_data_out;
		UINT resource_block_num = (TextureHeapAliaze * 4) / subresources_size;
		UINT copy_resource_block_num = (TextureHeapAliaze) / subresources_size;
		if (resource_block_num < 1)
		{
			resource_block_num = 1;
		}
		if (resource_block_num > 32)
		{
			resource_block_num = 32;
		}
		if (copy_resource_block_num < 1)
		{
			copy_resource_block_num = 1;
		}
		if (copy_resource_block_num > 16)
		{
			copy_resource_block_num = 16;
		}
		//填充资源格式
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", subresources_size);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", PancyJsonTool::GetInstance()->GetEnumName(typeid(heap_type).name(), heap_type));
		for (int i = 0; i < heap_flags.size(); ++i)
		{
			PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", PancyJsonTool::GetInstance()->GetEnumName(typeid(heap_flags[i]).name(), heap_flags[i]));
		}
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, heap_name);
		//将文件标记为已经创建
		FileBuildRepeatCheck::GetInstance()->AddFileName(heap_name);
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "repeat load json file: " + heap_name, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		return error_message;
	}
	if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(bufferblock_file_name))
	{
		//更新格式文件
		Json::Value json_data_resourceview;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", heap_name);
		Json::Value json_data_res_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", PancyJsonTool::GetInstance()->GetEnumName(typeid(subresource_desc.Dimension).name(), subresource_desc.Dimension));
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", subresource_desc.Width);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Height", subresource_desc.Height);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "DepthOrArraySize", subresource_desc.DepthOrArraySize);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "MipLevels", subresource_desc.MipLevels);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Format", PancyJsonTool::GetInstance()->GetEnumName(typeid(subresource_desc.Format).name(), subresource_desc.Format));
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Layout", PancyJsonTool::GetInstance()->GetEnumName(typeid(subresource_desc.Layout).name(), subresource_desc.Layout));
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Flags", PancyJsonTool::GetInstance()->GetEnumName(typeid(subresource_desc.Flags).name(), subresource_desc.Flags));
		Json::Value json_data_sample_desc;
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Count", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Quality", 0);
		//递归回调
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "SampleDesc", json_data_sample_desc);
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_DESC", json_data_res_desc);
		//继续填充主干
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", PancyJsonTool::GetInstance()->GetEnumName(typeid(res_state).name(), res_state));
		PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", subresources_size);
		//写入文件并标记为已创建
		PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, bufferblock_file_name);
		FileBuildRepeatCheck::GetInstance()->AddFileName(bufferblock_file_name);
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "repeat load json file: " + bufferblock_file_name, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		return error_message;
	}
	subresource_desc_name = bufferblock_file_name;
	return PancystarEngine::succeed;
}