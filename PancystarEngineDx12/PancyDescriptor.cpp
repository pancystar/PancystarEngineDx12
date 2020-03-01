#include"PancyDescriptor.h"
using namespace PancystarEngine;

//解绑定描述符段，用于描述符段里的所有描述符页的分配和管理
BindlessResourceViewSegmental::BindlessResourceViewSegmental(
	const pancy_object_id &max_descriptor_num_in,
	const pancy_object_id &segmental_offset_position_in,
	const pancy_object_id &per_descriptor_size_in,
	const ComPtr<ID3D12DescriptorHeap> descriptor_heap_data_in
)
{
	max_descriptor_num = max_descriptor_num_in;
	segmental_offset_position = segmental_offset_position_in;
	now_descriptor_pack_id_self_add = 0;
	now_pointer_offset = 0;
	per_descriptor_size = per_descriptor_size_in;
	descriptor_data.rehash(max_descriptor_num);
	now_pointer_refresh = max_descriptor_num_in;
	descriptor_heap_data = descriptor_heap_data_in.Get();
}
//根据描述符页的指针信息，在描述符堆开辟描述符
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::BuildShaderResourceView(const BindlessResourceViewPointer &resource_view_pointer)
{
	for (int i = 0; i < resource_view_pointer.resource_view_num; ++i)
	{
		//计算当前的描述符在整个描述符堆的偏移量(段首地址偏移+页首地址偏移+自偏移)
		pancy_object_id resource_view_heap_offset = segmental_offset_position + resource_view_pointer.resource_view_offset + i;
		//计算虚拟偏移量对应的真实地址偏移量
		pancy_resource_size real_offset = static_cast<pancy_resource_size>(resource_view_heap_offset) * static_cast<pancy_resource_size>(per_descriptor_size);
		//根据真实地址偏移量，创建SRV描述符
		PancystarEngine::EngineFailReason check_error;
		//创建描述符
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptor_heap_data->GetCPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(real_offset);
		//检测资源格式
		auto real_resource = resource_view_pointer.describe_memory_data[i].GetResourceData();
		ResourceBlockGpu* now_gpu_resource = NULL;
		if (real_resource->GetResourceTypeName() == typeid(PancyBasicBuffer).name()) 
		{
			const PancyBasicBuffer *buffer_resource_data = dynamic_cast<const PancyBasicBuffer*>(real_resource);
			now_gpu_resource = buffer_resource_data->GetGpuResourceData();
		}
		else if(real_resource->GetResourceTypeName() == typeid(PancyBasicTexture).name())
		{
			const PancyBasicTexture *texture_resource_data = dynamic_cast<const PancyBasicTexture*>(real_resource);
			now_gpu_resource = texture_resource_data->GetGpuResourceData();
		}
		else 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "The Resource is not a buffer/texture, could not build Descriptor for it");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindlessResourceViewSegmental::BuildShaderResourceView", error_message);
			return error_message;
		}
		if (now_gpu_resource == NULL) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "The GPU data of Resource is empty, could not build Descriptor for it");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindlessResourceViewSegmental::BuildShaderResourceView", error_message);
			return error_message;
		}
		BasicDescriptorDesc new_descriptor_desc;
		new_descriptor_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeShaderResourceView;
		new_descriptor_desc.shader_resource_view_desc = resource_view_pointer.SRV_desc[i];
		check_error = now_gpu_resource->BuildCommonDescriptorView(new_descriptor_desc, cpuHandle);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
//从描述符段里开辟一组bindless描述符页
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::BuildBindlessShaderResourceViewPack(
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
	const std::vector<VirtualResourcePointer> &describe_memory_data,
	const pancy_object_id &SRV_pack_size,
	pancy_object_id &SRV_pack_id
)
{
	PancystarEngine::EngineFailReason check_error;
	//检查当前空闲的描述符数量是否足以开辟出当前请求的描述符页
	pancy_object_id now_empty_descriptor = max_descriptor_num - now_pointer_offset;
	if (now_empty_descriptor < SRV_pack_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "The Descriptor Segmental Is Not Enough to build a new descriptor pack");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Descriptor pack from Segmental", error_message);
		return error_message;
	}
	//根据当前的描述符段的偏移情况，创建一个新的描述符组
	BindlessResourceViewPointer new_resource_view_pack;
	new_resource_view_pack.resource_view_num = SRV_pack_size;
	new_resource_view_pack.resource_view_offset = now_pointer_offset;
	for (auto SRV_desc_data = SRV_desc.begin(); SRV_desc_data != SRV_desc.end(); ++SRV_desc_data)
	{
		//复制所有的描述符格式
		new_resource_view_pack.SRV_desc.push_back(*SRV_desc_data);
	}
	for (auto resource_data = describe_memory_data.begin(); resource_data != describe_memory_data.end(); ++resource_data)
	{
		//复制所有的资源二级地址
		new_resource_view_pack.describe_memory_data.push_back(*resource_data);
	}
	//开始创建SRV
	check_error = BuildShaderResourceView(new_resource_view_pack);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//根据新的描述符页所包含的描述符数量来偏移描述符段的指针
	now_pointer_offset += SRV_pack_size;
	//为新的描述符来生成一个新的ID
	if (!now_descriptor_pack_id_reuse.empty())
	{
		SRV_pack_id = now_descriptor_pack_id_reuse.front();
		now_descriptor_pack_id_reuse.pop();
	}
	else
	{
		SRV_pack_id = now_descriptor_pack_id_self_add;
		now_descriptor_pack_id_self_add += 1;
	}
	//将新生成的数据添加到表单进行记录
	descriptor_data.insert(std::pair<pancy_resource_id, BindlessResourceViewPointer>(SRV_pack_id, new_resource_view_pack));
	return PancystarEngine::succeed;
}
//从描述符段里删除一组bindless描述符页
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::DeleteBindlessShaderResourceViewPack(const pancy_object_id &SRV_pack_id)
{
	//找到当前描述符页的所在区域
	auto descriptor_page = descriptor_data.find(SRV_pack_id);
	if (descriptor_page == descriptor_data.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find the descriptor page ID: " + std::to_string(SRV_pack_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete descriptor page from Segmental", error_message);
		return error_message;
	}
	if (descriptor_page->second.resource_view_offset < now_pointer_refresh)
	{
		//如果当前删除的描述符页处于描述符段的靠前位置，则标记下一次整理描述符碎片的时候从当前位置开始整理
		now_pointer_refresh = descriptor_page->second.resource_view_offset;
	}
	//销毁当前描述符页
	descriptor_data.erase(SRV_pack_id);
	return PancystarEngine::succeed;
}
//为描述符段执行一次碎片整理操作
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::RefreshBindlessShaderResourceViewPack()
{
	PancystarEngine::EngineFailReason check_error;
	if (now_pointer_refresh >= max_descriptor_num)
	{
		//在此之前描述符段没有经过任何删除操作，不需要进行碎片整理
		return PancystarEngine::succeed;
	}
	now_pointer_offset = now_pointer_refresh;
	//遍历描述符段内的所有描述符页进行描述符碎片的整理
	for (auto descriptor_page_check = descriptor_data.begin(); descriptor_page_check != descriptor_data.end(); ++descriptor_page_check)
	{
		if (descriptor_page_check->second.resource_view_offset > now_pointer_refresh)
		{
			//如果该描述符页处于需要调整的位置则对这一页的所有描述符进行调整
			descriptor_page_check->second.resource_view_offset = now_pointer_offset;
			//生成新的SRV
			check_error = BuildShaderResourceView(descriptor_page_check->second);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//标记当前被占用的描述符位置
			now_pointer_offset += descriptor_page_check->second.resource_view_num;
			if (now_pointer_offset > max_descriptor_num)
			{
				//调整过程中出现调整后描述符数量反而高于调整前的异常情况
				PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor Segmental could not build desciptor more than : " + std::to_string(max_descriptor_num));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Refresh descriptor page from Segmental", error_message);
				return error_message;
			}
		}
	}
	//还原刷新调整
	now_pointer_refresh = max_descriptor_num;
	return PancystarEngine::succeed;
}
//为描述符段删除一组bindless描述符页，同时整理一次描述符碎片
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::DeleteBindlessShaderResourceViewPackAndRefresh(const pancy_object_id &SRV_pack_id)
{
	auto check_error = DeleteBindlessShaderResourceViewPack(SRV_pack_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = RefreshBindlessShaderResourceViewPack();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
const BindlessResourceViewPointer BindlessResourceViewSegmental::GetDescriptorPageOffset(const pancy_object_id &descriptor_page_id)
{
	auto check_descriptor_page = descriptor_data.find(descriptor_page_id);
	if (check_descriptor_page == descriptor_data.end())
	{
		BindlessResourceViewPointer error_pointer;
		error_pointer.resource_view_offset = 0;
		error_pointer.resource_view_num = 0;
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find the descriptor page ID: " + std::to_string(descriptor_page_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get descriptor page from Segmental", error_message);
		return error_pointer;
	}
	return check_descriptor_page->second;
}
//用于描述符堆资源的容量排序
bool BindlessDescriptorID::operator<(const BindlessDescriptorID& other)  const
{
	if (empty_resource_size != other.empty_resource_size)
	{
		return (empty_resource_size < other.empty_resource_size);
	}
	return (bindless_id < other.bindless_id);
}
//描述符堆，用于从描述符上分配描述符
PancyDescriptorHeap::PancyDescriptorHeap()
{


}
PancystarEngine::EngineFailReason PancyDescriptorHeap::Create(
	const D3D12_DESCRIPTOR_HEAP_DESC &descriptor_heap_desc,
	const std::string &descriptor_heap_name_in,
	const pancy_object_id &bind_descriptor_num_in,
	const pancy_object_id &bindless_descriptor_num_in,
	const pancy_object_id &per_segmental_size_in
)
{
	descriptor_desc = descriptor_heap_desc;
	descriptor_heap_name = descriptor_heap_name_in;
	bind_descriptor_num = bind_descriptor_num_in;
	bindless_descriptor_num = bindless_descriptor_num_in;
	per_segmental_size = per_segmental_size_in;
	//计算每一个描述符的大小
	per_descriptor_size = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetDescriptorHandleIncrementSize(descriptor_desc.Type);
	//根据描述符堆格式创建描述符堆
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "create descriptor heap error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap", error_message);
		return error_message;
	}
	//初始化所有的全局描述符ID
	for (int i = 0; i < bind_descriptor_num; ++i)
	{
		bind_descriptor_offset_reuse.push(i);
	}
	//为bindless描述符区域初始化每个描述符段
	pancy_object_id globel_offset = bind_descriptor_num;
	pancy_object_id segmantal_id_self_add = 0;
	for (int i = 0; i < bindless_descriptor_num; i += per_segmental_size)
	{
		BindlessResourceViewSegmental *new_segmental = new BindlessResourceViewSegmental(per_segmental_size, globel_offset + i, per_descriptor_size, descriptor_heap_data);
		if (new_segmental == NULL)
		{
			//开辟描述符段失败
			PancystarEngine::EngineFailReason error_message(E_FAIL, "Build bindless texture segmental failed with NULL return");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build bindless texture segmental", error_message);
			return error_message;
		}
		if (new_segmental->GetEmptyDescriptorNum() != per_segmental_size)
		{
			//开辟描述符段得到的描述符数量不等于预期的描述符数量
			PancystarEngine::EngineFailReason error_message(E_FAIL, "Build bindless texture segmental failed with Wrong dscriptor num,ask: " + std::to_string(per_segmental_size) + " but find: " + std::to_string(new_segmental->GetEmptyDescriptorNum()));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build bindless texture segmental", error_message);
			return error_message;
		}
		//将新的描述符段保留到存储描述符段的表中
		BindlessDescriptorID new_descriptor_segmental_id;
		new_descriptor_segmental_id.bindless_id = segmantal_id_self_add;
		new_descriptor_segmental_id.empty_resource_size = per_segmental_size;
		bindless_descriptor_id_map.insert(std::pair<pancy_object_id, BindlessDescriptorID>(new_descriptor_segmental_id.bindless_id, new_descriptor_segmental_id));
		descriptor_segmental_map.insert(std::pair<BindlessDescriptorID, BindlessResourceViewSegmental*>(new_descriptor_segmental_id, new_segmental));
		segmantal_id_self_add += 1;
	}
	return PancystarEngine::succeed;
}
PancyDescriptorHeap::~PancyDescriptorHeap()
{
	for (auto release_data = descriptor_segmental_map.begin(); release_data != descriptor_segmental_map.end(); ++release_data)
	{
		delete release_data->second;
	}
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildBindlessShaderResourceViewPage(
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
	const std::vector<VirtualResourcePointer> &describe_memory_data,
	const pancy_object_id &SRV_pack_size,
	BindlessResourceViewID &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	if (SRV_pack_size <= 0)
	{
		//需要创建的SRV数量小于等于0
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not Build bindless texture with size:" + SRV_pack_size);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build SRV for bindless texture", error_message);
		return error_message;
	}
	//先挑选一个合适的bindless描述符段(要求剩余描述符足够，且尽量在其他描述符中的空余值最小)
	BindlessResourceViewSegmental* RSV_segmental = NULL;
	BindlessDescriptorID check_min_size_id;
	check_min_size_id.bindless_id = 0;
	check_min_size_id.empty_resource_size = SRV_pack_size;
	auto min_resource_data = descriptor_segmental_map.lower_bound(check_min_size_id);
	if (min_resource_data != descriptor_segmental_map.end())
	{
		//描述符堆中存在符合要求的描述符段，直接在该描述符段上开辟描述符页
		descriptor_id.segmental_id = min_resource_data->first.bindless_id;
		RSV_segmental = min_resource_data->second;
	}
	else
	{
		//描述符堆已满
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not Build bindless texture,the heap is full，ask number: " + std::to_string(SRV_pack_size));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build SRV for bindless texture", error_message);
		return error_message;
	}
	//在描述符段上开辟一个描述符页
	check_error = RSV_segmental->BuildBindlessShaderResourceViewPack(SRV_desc, describe_memory_data, SRV_pack_size, descriptor_id.page_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//修改描述符页的大小(先从map中删除老的描述符页，再将新的描述符页插入到map中,实现通过Map维护描述符页大小的功能)
	check_error = RefreshBindlessResourcesegmentalSize(min_resource_data->first.bindless_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::RefreshBindlessResourcesegmentalSize(const pancy_object_id &resourc_id)
{

	auto now_resource_data = bindless_descriptor_id_map.find(resourc_id);
	if (now_resource_data == bindless_descriptor_id_map.end())
	{
		//需要刷新的资源id不存在
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless resource segmental id" + std::to_string(resourc_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Refresh bindless SRV size", error_message);
		return error_message;
	}
	auto RSV_segmental = descriptor_segmental_map.find(now_resource_data->second);
	if (RSV_segmental == descriptor_segmental_map.end())
	{
		//需要刷新的资源不存在
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless resource segmental resource" + std::to_string(resourc_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Refresh bindless SRV size", error_message);
		return error_message;
	}
	//记录当前需要更替的资源的新地址与旧地址
	BindlessDescriptorID old_size_id;
	BindlessDescriptorID new_size_id;
	BindlessResourceViewSegmental *segmental_resource;
	new_size_id.bindless_id = resourc_id;
	new_size_id.empty_resource_size = RSV_segmental->second->GetEmptyDescriptorNum();
	old_size_id = now_resource_data->second;
	segmental_resource = RSV_segmental->second;
	//刷新ID
	bindless_descriptor_id_map.erase(old_size_id.bindless_id);
	bindless_descriptor_id_map.insert(std::pair<pancy_object_id, BindlessDescriptorID>(new_size_id.bindless_id, new_size_id));
	//刷新内容
	descriptor_segmental_map.erase(old_size_id);
	descriptor_segmental_map.insert(std::pair<BindlessDescriptorID, BindlessResourceViewSegmental*>(new_size_id, segmental_resource));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::DeleteBindlessShaderResourceViewPage(
	const BindlessResourceViewID &descriptor_id,
	bool is_refresh_segmental
)
{
	auto resource_id = bindless_descriptor_id_map.find(descriptor_id.segmental_id);
	if (resource_id == bindless_descriptor_id_map.end())
	{
		//未找到请求的资源
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(descriptor_id.segmental_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete bindless SRV from segmental", error_message);
		return error_message;
	}
	auto descriptor_resource = descriptor_segmental_map.find(resource_id->second);
	if (descriptor_resource == descriptor_segmental_map.end())
	{
		//未找到请求的资源
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(descriptor_id.segmental_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete bindless SRV from segmental", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error;
	if (is_refresh_segmental)
	{
		//要求删除页信息后立即刷新段信息
		check_error = descriptor_resource->second->DeleteBindlessShaderResourceViewPackAndRefresh(descriptor_id.page_id);
	}
	else
	{
		//删除页信息后不立即刷新段信息
		check_error = descriptor_resource->second->DeleteBindlessShaderResourceViewPack(descriptor_id.page_id);
	}
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//重新计算页的大小并刷新页表
	check_error = RefreshBindlessResourcesegmentalSize(descriptor_id.segmental_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::RefreshBindlessShaderResourceViewSegmental()
{
	PancystarEngine::EngineFailReason check_error;
	int size = bindless_descriptor_id_map.size();
	for (int i = 0; i < size; ++i)
	{
		BindlessDescriptorID new_id = bindless_descriptor_id_map[i];
		auto bindlessresource = descriptor_segmental_map.find(new_id);
		if (bindlessresource == descriptor_segmental_map.end())
		{
			//未找到请求的资源
			PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(i));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Refresh bindless SRV segmental", error_message);
			return error_message;
		}
		check_error = bindlessresource->second->RefreshBindlessShaderResourceViewPack();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = RefreshBindlessResourcesegmentalSize(i);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
//创建全局描述符
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildGlobelDescriptor(
	const std::string &globel_name,
	const std::vector<BasicDescriptorDesc> &SRV_desc,
	const std::vector <VirtualResourcePointer>& memory_data,
	const bool if_build_multi_buffer
)
{
	auto check_if_has_data = descriptor_globel_map.find(globel_name);
	//先检查是否已经创建了同名的全局描述符
	if (check_if_has_data != descriptor_globel_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "repeat build globel descriptor: " + globel_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build globel descriptor from heap", error_message);
		return error_message;
	}
	//创建一个普通的绑定描述符
	pancy_object_id bind_resource_id;
	PancystarEngine::EngineFailReason check_error;
	check_error = BuildBindDescriptor(SRV_desc, memory_data, if_build_multi_buffer, bind_resource_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//将绑定描述符与全局变量进行绑定
	descriptor_globel_map[globel_name] = bind_resource_id;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::DeleteGlobelDescriptor(const std::string &globel_name)
{
	PancystarEngine::EngineFailReason check_error;
	auto check_if_has_data = descriptor_globel_map.find(globel_name);
	//先检查是否已经创建了全局描述符
	if (check_if_has_data == descriptor_globel_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find globel descriptor: " + globel_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete globel descriptor from heap", error_message);
		return error_message;
	}
	//先删除描述符资源
	check_error = DeleteBindDescriptor(check_if_has_data->second);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//再删除描述符资源的命名关联
	descriptor_globel_map.erase(globel_name);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::GetGlobelDesciptorID(const std::string &globel_name, pancy_object_id &descriptor_id)
{
	PancystarEngine::EngineFailReason check_error;
	auto check_if_has_data = descriptor_globel_map.find(globel_name);
	//先检查是否已经创建了全局描述符
	if (check_if_has_data == descriptor_globel_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find globel descriptor: " + globel_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get globel descriptor from heap", error_message);
		return error_message;
	}
	descriptor_id = check_if_has_data->second;
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildBindDescriptor(
	const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
	const std::vector<VirtualResourcePointer>& memory_data,
	const bool if_build_multi_buffer,
	pancy_object_id &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> cpuHandle;
	CommonDescriptorPointer new_descriptor_data;
	//检测当前的描述符堆格式是否与期望的描述符一致
	D3D12_DESCRIPTOR_HEAP_TYPE now_descriptor_heap = GetDescriptorHeapTypeOfDescriptor(now_descriptor_desc_in[0]);
	if (descriptor_desc.Type != now_descriptor_heap)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap type is not same as heap, could not build bind descriptor: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build bind descriptor from heap", error_message);
		return error_message;
	}
	//预处理描述符在描述符堆中的位置
	check_error = PreBuildBindDescriptor(
		now_descriptor_heap,
		if_build_multi_buffer,
		cpuHandle,
		new_descriptor_data
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (cpuHandle.size() != memory_data.size())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "descriptor_number: " + std::to_string(cpuHandle.size()) + "do not match resource number: " + std::to_string(memory_data.size()) + " checking if need to build multi buffer");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeap::BuildBindDescriptor", error_message);
		return error_message;
	}
	//创建描述符
	for (int i = 0; i < memory_data.size(); ++i)
	{
		//存储资源数据，并根据资源数据创建描述符
		new_descriptor_data.resource_data.push_back(memory_data[i]);
		auto real_resource = memory_data[i].GetResourceData();
		ResourceBlockGpu* now_gpu_resource = NULL;
		if (real_resource->GetResourceTypeName() == typeid(PancyBasicBuffer).name())
		{
			const PancyBasicBuffer *buffer_resource_data = dynamic_cast<const PancyBasicBuffer*>(real_resource);
			now_gpu_resource = buffer_resource_data->GetGpuResourceData();
		}
		else if (real_resource->GetResourceTypeName() == typeid(PancyBasicTexture).name())
		{
			const PancyBasicTexture *texture_resource_data = dynamic_cast<const PancyBasicTexture*>(real_resource);
			now_gpu_resource = texture_resource_data->GetGpuResourceData();
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "The Resource is not a buffer/texture, could not build Descriptor for it");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindlessResourceViewSegmental::BuildShaderResourceView", error_message);
			return error_message;
		}
		if (now_gpu_resource == NULL)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "The GPU data of Resource is empty, could not build Descriptor for it");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindlessResourceViewSegmental::BuildShaderResourceView", error_message);
			return error_message;
		}
		check_error = now_gpu_resource->BuildCommonDescriptorView(now_descriptor_desc_in[i], cpuHandle[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	//将新创建的描述符加入到map中
	descriptor_id = new_descriptor_data.descriptor_offset[0];
	descriptor_bind_map[descriptor_id] = new_descriptor_data;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::PreBuildBindDescriptor(
	const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_type,
	const bool if_build_multi_buffer,
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> &descriptor_cpu_handle,
	CommonDescriptorPointer &new_descriptor_data
)
{
	pancy_object_id multibuffer_num = static_cast<pancy_object_id>(PancyDx12DeviceBasic::GetInstance()->GetFrameNum());
	PancystarEngine::EngineFailReason check_error;
	//检测是否还有空余的描述符位置
	if (bind_descriptor_offset_reuse.size() == 0 || (if_build_multi_buffer && bind_descriptor_offset_reuse.size() < multibuffer_num))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap is full, could not build new bind descriptor: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build globel descriptor from heap", error_message);
		return error_message;
	}
	//检测需要创建的描述符数量(允许创建多个缓冲区则置1，否则根据当前程序渲染帧的数量创建多组资源)
	pancy_object_id build_descriptor_num = 1;
	if (if_build_multi_buffer)
	{
		build_descriptor_num = multibuffer_num;
		new_descriptor_data.if_multi_buffer = true;
	}
	//根据需要创建的描述符的数量来计算每个描述符在描述符堆内的真正偏移量
	descriptor_cpu_handle.clear();
	for (int i = 0; i < build_descriptor_num; ++i)
	{
		new_descriptor_data.descriptor_offset.push_back(bind_descriptor_offset_reuse.front());
		new_descriptor_data.descriptor_type = descriptor_type;
		//计算描述符真正的位置
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptor_heap_data->GetCPUDescriptorHandleForHeapStart());
		pancy_resource_size real_descriptor_offset = static_cast<pancy_resource_size>(bind_descriptor_offset_reuse.front()) * static_cast<pancy_resource_size>(per_descriptor_size);
		cpuHandle.Offset(real_descriptor_offset);
		descriptor_cpu_handle.push_back(cpuHandle);
		//删除当前可用的一个ID号
		bind_descriptor_offset_reuse.pop();
	}
	return PancystarEngine::succeed;
}
D3D12_DESCRIPTOR_HEAP_TYPE PancyDescriptorHeap::GetDescriptorHeapTypeOfDescriptor(const BasicDescriptorDesc &descriptor_desc)
{
	switch (descriptor_desc.basic_descriptor_type)
	{
	case PancyDescriptorType::DescriptorTypeShaderResourceView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		break;
	case PancyDescriptorType::DescriptorTypeUnorderedAccessView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		break;
	case PancyDescriptorType::DescriptorTypeConstantBufferView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		break;
	case PancyDescriptorType::DescriptorTypeRenderTargetView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		break;
	case PancyDescriptorType::DescriptorTypeDepthStencilView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		break;
	default:
		break;
	}
	return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::DeleteBindDescriptor(const pancy_object_id &descriptor_id)
{
	auto check_if_has_data = descriptor_bind_map.find(descriptor_id);
	//先检查待删除的描述符是否存在
	if (check_if_has_data == descriptor_bind_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find bind descriptor: " + std::to_string(descriptor_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete bind descriptor from heap", error_message);
		return error_message;
	}
	//删除当前的描述符数据
	descriptor_bind_map.erase(descriptor_id);
	//将删除完毕的描述符ID还给描述符ID池再利用
	bind_descriptor_offset_reuse.push(descriptor_id);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BindGlobelDescriptor(
	const std::string &globel_name,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	//先获取描述符堆的CPU指针
	pancy_object_id bind_id_descriptor;
	check_error = GetGlobelDesciptorID(globel_name, bind_id_descriptor);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//获取全局描述符的偏移量
	check_error = BindCommonDescriptor(bind_id_descriptor, render_param_type, m_commandList, root_signature_offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BindCommonDescriptor(
	const pancy_object_id &descriptor_id,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	//检测输入的ID号是否合法
	if (descriptor_id >= bind_descriptor_num)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor id: " + std::to_string(descriptor_id) + " is bigger than the max id of descriptor heap: " + std::to_string(bind_descriptor_num));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindCommonDescriptor", error_message);
		return error_message;
	}
	//先获取描述符堆的CPU指针
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(descriptor_heap_data->GetGPUDescriptorHandleForHeapStart());
	//获取全局描述符的信息
	auto resource_id = descriptor_bind_map.find(descriptor_id);
	if (resource_id == descriptor_bind_map.end())
	{
		//未找到请求的资源
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bind descriptor :" + std::to_string(descriptor_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeap::BindCommonDescriptor", error_message);
		return error_message;
	}
	//根据描述符是否是交换帧类型来决定是否切帧（即老版本的rename操作）
	pancy_object_id real_offset_descriptor;
	if (resource_id->second.if_multi_buffer)
	{
		pancy_object_id now_frame_id = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
		real_offset_descriptor = resource_id->second.descriptor_offset[now_frame_id];
	}
	else
	{
		real_offset_descriptor = resource_id->second.descriptor_offset[0];
	}
	//获取全局描述符的偏移量
	pancy_object_id id_offset = real_offset_descriptor * per_descriptor_size;
	srvHandle.Offset(id_offset);
	//开始绑定描述符与commandlist
	switch (render_param_type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(root_signature_offset, srvHandle);
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		m_commandList->GetCommandList()->SetComputeRootDescriptorTable(root_signature_offset, srvHandle);
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could only bind descriptor to graph/cpmpute commondlist:" + std::to_string(descriptor_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeap::BindCommonDescriptor", error_message);
		return error_message;
		break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::GetCommonDescriptorCpuOffset(const pancy_object_id &descriptor_id, CD3DX12_CPU_DESCRIPTOR_HANDLE &Cpu_Handle)
{
	//检测输入的ID号是否合法
	if (descriptor_id >= bind_descriptor_num)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor id: " + std::to_string(descriptor_id) + " is bigger than the max id of descriptor heap: " + std::to_string(bind_descriptor_num));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindCommonDescriptor", error_message);
		return error_message;
	}
	//先获取描述符堆的CPU指针
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(descriptor_heap_data->GetCPUDescriptorHandleForHeapStart());
	//获取全局描述符的信息
	auto resource_id = descriptor_bind_map.find(descriptor_id);
	if (resource_id == descriptor_bind_map.end())
	{
		//未找到请求的资源
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bind descriptor :" + std::to_string(descriptor_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Bind bind descriptor from segmental", error_message);
		return error_message;
	}
	//根据描述符是否是交换帧类型来决定是否切帧（即老版本的rename操作）
	pancy_object_id real_offset_descriptor;
	if (resource_id->second.if_multi_buffer)
	{
		pancy_object_id now_frame_id = PancyDx12DeviceBasic::GetInstance()->GetLastFrame();
		real_offset_descriptor = resource_id->second.descriptor_offset[now_frame_id];
	}
	else
	{
		real_offset_descriptor = resource_id->second.descriptor_offset[0];
	}
	//获取全局描述符的偏移量
	pancy_object_id id_offset = real_offset_descriptor * per_descriptor_size;
	srvHandle.Offset(id_offset);
	Cpu_Handle = srvHandle;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BindBindlessDescriptor(
	const BindlessResourceViewID &descriptor_id,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	//先获取描述符堆的CPU指针
	if (descriptor_id.segmental_id >= bindless_descriptor_num)
	{
		//请求的描述符段位置超过了段地址的最大值
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the bindless descriptor segmental id: " + std::to_string(descriptor_id.segmental_id) + " is bigger than the max bindless segmental id of descriptor heap: " + std::to_string(bindless_descriptor_num));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindBindlessDescriptor", error_message);
		return error_message;
	}
	else if (descriptor_id.page_id > per_segmental_size)
	{
		//请求的描述符页位置超过了每段地址的最大容量
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the bindless descriptor page id: " + std::to_string(descriptor_id.page_id) + " is bigger than the max bindless per_segmental_size id of descriptor heap: " + std::to_string(per_segmental_size));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindBindlessDescriptor", error_message);
		return error_message;
	}
	pancy_object_id bind_id_descriptor;
	//查找当前描述符页对应的数据
	auto resource_id = bindless_descriptor_id_map.find(descriptor_id.segmental_id);
	if (resource_id == bindless_descriptor_id_map.end())
	{
		//未找到请求的资源
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(descriptor_id.segmental_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Bind bindless SRV from segmental", error_message);
		return error_message;
	}
	auto descriptor_resource = descriptor_segmental_map.find(resource_id->second);
	if (descriptor_resource == descriptor_segmental_map.end())
	{
		//未找到请求的资源
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(descriptor_id.segmental_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Bind bindless SRV from segmental", error_message);
		return error_message;
	}
	//获取描述符页的真实偏移
	const BindlessResourceViewPointer &descriptor_page_real_pos = descriptor_resource->second->GetDescriptorPageOffset(descriptor_id.page_id);
	//计算解绑顶描述符的起始位置在描述符堆中的偏移（初始偏移+所有段间的偏移+段内偏移）
	bind_id_descriptor = bind_descriptor_num + descriptor_id.segmental_id * per_segmental_size + descriptor_page_real_pos.resource_view_offset;
	//先获取描述符堆的CPU指针
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(descriptor_heap_data->GetGPUDescriptorHandleForHeapStart());
	//获取全局描述符的偏移量
	pancy_object_id id_offset = bind_id_descriptor * per_descriptor_size;
	srvHandle.Offset(id_offset);
	//开始绑定描述符与commandlist
	switch (render_param_type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(root_signature_offset, srvHandle);
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		m_commandList->GetCommandList()->SetComputeRootDescriptorTable(root_signature_offset, srvHandle);
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could only bind descriptor to graph/cpmpute commondlist");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeap::BindBindlessDescriptor", error_message);
		return error_message;
		break;
	}
	return PancystarEngine::succeed;
}
//描述符堆管理器
PancyDescriptorHeapControl::PancyDescriptorHeapControl()
{
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	//创建三个基础的描述符堆
	PancystarEngine::EngineFailReason check_error;
	check_error = BuildNewDescriptorHeapFromJson("EngineResource\\BasicDescriptorHeap\\DesciptorHeapShaderResource.json", common_descriptor_heap_shader_resource);
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init Descriptor Heap Controler", check_error);
	}
	check_error = BuildNewDescriptorHeapFromJson("EngineResource\\BasicDescriptorHeap\\DesciptorHeapRenderTarget.json", common_descriptor_heap_render_target);
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init Descriptor Heap Controler", check_error);
	}
	check_error = BuildNewDescriptorHeapFromJson("EngineResource\\BasicDescriptorHeap\\DesciptorHeapDepthStencil.json", common_descriptor_heap_depth_stencil);
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init Descriptor Heap Controler", check_error);
	}
}
PancyDescriptorHeapControl::~PancyDescriptorHeapControl()
{
	for (auto release_heap = descriptor_heap_map.begin(); release_heap != descriptor_heap_map.end(); ++release_heap)
	{
		delete release_heap->second;
	}
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildNewDescriptorHeapFromJson(const std::string &json_name, const Json::Value &root_value, pancy_resource_id &descriptor_heap_id)
{
	PancystarEngine::EngineFailReason check_error;
	PancyDescriptorHeap *descriptor_SRV = new PancyDescriptorHeap();
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc;
	pancy_json_value value_root;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "Flags", pancy_json_data_type::json_data_enum, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.Flags = static_cast<D3D12_DESCRIPTOR_HEAP_FLAGS>(value_root.int_value);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "NodeMask", pancy_json_data_type::json_data_int, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.NodeMask = value_root.int_value;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "Type", pancy_json_data_type::json_data_enum, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(value_root.int_value);
	pancy_object_id bind_descriptor_num;
	pancy_object_id bindless_descriptor_num;
	pancy_object_id per_segmental_size;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "BindDescriptorNum", pancy_json_data_type::json_data_int, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	bind_descriptor_num = static_cast<pancy_object_id>(value_root.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "BindlessDescriptorNum", pancy_json_data_type::json_data_int, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	bindless_descriptor_num = static_cast<pancy_object_id>(value_root.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "PerSegmentalSize", pancy_json_data_type::json_data_int, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	per_segmental_size = static_cast<pancy_object_id>(value_root.int_value);
	descriptor_heap_desc.NumDescriptors = bind_descriptor_num + bindless_descriptor_num;
	check_error = descriptor_SRV->Create(descriptor_heap_desc, json_name, bind_descriptor_num, bindless_descriptor_num, per_segmental_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (!descriptor_heap_id_reuse.empty())
	{
		descriptor_heap_id = descriptor_heap_id_reuse.front();
		descriptor_heap_id_reuse.pop();
	}
	else
	{
		descriptor_heap_id = descriptor_heap_id_self_add;
		descriptor_heap_id_self_add += 1;
	}
	descriptor_heap_map[descriptor_heap_id] = descriptor_SRV;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildNewDescriptorHeapFromJson(const std::string &json_file_name, pancy_resource_id &descriptor_heap_id)
{
	PancystarEngine::EngineFailReason check_error;
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(json_file_name, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = BuildNewDescriptorHeapFromJson(json_file_name, root_value, descriptor_heap_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::DeleteDescriptorHeap(const pancy_resource_id &descriptor_heap_id)
{
	auto descriptor_data = descriptor_heap_map.find(descriptor_heap_id);
	if (descriptor_data == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find descriptor heap: " + std::to_string(descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete descriptor heap from heap", error_message);
		return error_message;
	}
	//删除描述符堆资源，并将ID号还给描述符堆管理器
	delete descriptor_heap_map[descriptor_heap_id];
	descriptor_heap_map.erase(descriptor_heap_id);
	descriptor_heap_id_reuse.push(descriptor_heap_id);
	return PancystarEngine::succeed;
}
pancy_resource_id PancyDescriptorHeapControl::GetCommonDescriptorHeapID(const BasicDescriptorDesc &descriptor_desc)
{
	return GetCommonDescriptorHeapID(descriptor_desc.basic_descriptor_type);
}
pancy_resource_id PancyDescriptorHeapControl::GetCommonDescriptorHeapID(const PancyDescriptorType &descriptor_type)
{
	switch (descriptor_type)
	{
	case PancyDescriptorType::DescriptorTypeShaderResourceView:
		return common_descriptor_heap_shader_resource;
		break;
	case PancyDescriptorType::DescriptorTypeUnorderedAccessView:
		return common_descriptor_heap_shader_resource;
		break;
	case PancyDescriptorType::DescriptorTypeConstantBufferView:
		return common_descriptor_heap_shader_resource;
		break;
	case PancyDescriptorType::DescriptorTypeRenderTargetView:
		return common_descriptor_heap_render_target;
		break;
	case PancyDescriptorType::DescriptorTypeDepthStencilView:
		return common_descriptor_heap_depth_stencil;
		break;
	default:
		break;
	}
	return common_descriptor_heap_shader_resource;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildCommonDescriptor(
	const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
	const std::vector<VirtualResourcePointer>& memory_data,
	const bool if_build_multi_buffer,
	BindDescriptorPointer &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_used_descriptor_heap = GetCommonDescriptorHeapID(now_descriptor_desc_in[0]);
	auto common_descriptor_heap = descriptor_heap_map.find(now_used_descriptor_heap);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	descriptor_id.descriptor_heap_id = now_used_descriptor_heap;
	check_error = common_descriptor_heap->second->BuildBindDescriptor(now_descriptor_desc_in, memory_data, if_build_multi_buffer, descriptor_id.descriptor_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildCommonBindlessShaderResourceView(
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
	const std::vector<VirtualResourcePointer> &describe_memory_data,
	const pancy_object_id &SRV_pack_size,
	BindlessDescriptorPointer &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	auto common_srv_heap = descriptor_heap_map.find(common_descriptor_heap_shader_resource);
	if (common_srv_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic SRV descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build SRV descriptor heap from heap", error_message);
		return error_message;
	}
	descriptor_id.descriptor_heap_id = common_descriptor_heap_shader_resource;
	check_error = common_srv_heap->second->BuildBindlessShaderResourceViewPage(SRV_desc, describe_memory_data, SRV_pack_size, descriptor_id.descriptor_pack_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildCommonGlobelDescriptor(
	const std::string &globel_srv_name,
	const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
	const std::vector<VirtualResourcePointer>& memory_data,
	const bool if_build_multi_buffer
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_used_descriptor_heap = GetCommonDescriptorHeapID(now_descriptor_desc_in[0]);
	auto common_descriptor_heap = descriptor_heap_map.find(now_used_descriptor_heap);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->BuildGlobelDescriptor(globel_srv_name, now_descriptor_desc_in, memory_data, if_build_multi_buffer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::GetCommonGlobelDescriptorID(
	PancyDescriptorType basic_descriptor_type,
	const std::string &globel_srv_name,
	BindDescriptorPointer &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_used_descriptor_heap = GetCommonDescriptorHeapID(basic_descriptor_type);
	auto common_descriptor_heap = descriptor_heap_map.find(now_used_descriptor_heap);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	descriptor_id.descriptor_heap_id = now_used_descriptor_heap;
	check_error = common_descriptor_heap->second->GetGlobelDesciptorID(globel_srv_name, descriptor_id.descriptor_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::GetBasicDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_heap_type, ID3D12DescriptorHeap **descriptor_heap_out)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id common_descriptor_heap_id = 999999999;
	switch (descriptor_heap_type)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		common_descriptor_heap_id = common_descriptor_heap_shader_resource;
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		common_descriptor_heap_id = common_descriptor_heap_render_target;
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		common_descriptor_heap_id = common_descriptor_heap_depth_stencil;
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not get common descriptor heap,type not defined");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Common Descriptor Heap", error_message);
		return error_message;
		break;
	}
	auto common_descriptor_heap = descriptor_heap_map.find(common_descriptor_heap_id);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Common Descriptor Heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->GetDescriptorHeapData(descriptor_heap_out);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BindCommonGlobelDescriptor(
	PancyDescriptorType basic_descriptor_type,
	const std::string &globel_name,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_used_descriptor_heap = GetCommonDescriptorHeapID(basic_descriptor_type);
	auto common_descriptor_heap = descriptor_heap_map.find(now_used_descriptor_heap);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->BindGlobelDescriptor(globel_name, render_param_type, m_commandList, root_signature_offset);
	if (check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BindCommonDescriptor(
	const BindDescriptorPointer &descriptor_id,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	auto common_descriptor_heap = descriptor_heap_map.find(descriptor_id.descriptor_heap_id);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find descriptor heap ID: " + std::to_string(descriptor_id.descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->BindCommonDescriptor(descriptor_id.descriptor_id, render_param_type, m_commandList, root_signature_offset);
	if (check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BindBindlessDescriptor(
	const BindlessDescriptorPointer &descriptor_id,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	auto common_descriptor_heap = descriptor_heap_map.find(descriptor_id.descriptor_heap_id);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find descriptor heap ID: " + std::to_string(descriptor_id.descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->BindBindlessDescriptor(descriptor_id.descriptor_pack_id, render_param_type, m_commandList, root_signature_offset);
	if (check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BindCommonRenderTargetUncontiguous(
	const std::vector<pancy_object_id> rendertarget_list,
	const pancy_object_id depthstencil_descriptor,
	PancyRenderCommandList *m_commandList,
	const bool &if_have_rendertarget,
	const bool &if_have_depthstencil
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id rtv_number = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE *rtvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto render_target_heap = descriptor_heap_map.find(common_descriptor_heap_render_target);
	auto depth_stencil_heap = descriptor_heap_map.find(common_descriptor_heap_depth_stencil);
	if (render_target_heap == descriptor_heap_map.end() || depth_stencil_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "haven't init common descriptor heap");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeapControl::BindCommonRenderTargetUncontiguous", error_message);
		return error_message;
	}
	//获取所有渲染目标的偏移量
	if (!if_have_rendertarget)
	{
		rtvHandle = NULL;
	}
	else
	{
		rtvHandle = new CD3DX12_CPU_DESCRIPTOR_HANDLE[rendertarget_list.size()];
		for (int i = 0; i < rendertarget_list.size(); ++i)
		{
			check_error = render_target_heap->second->GetCommonDescriptorCpuOffset(rendertarget_list[i], rtvHandle[i]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		rtv_number = rendertarget_list.size();
	}
	//获取深度缓冲区偏移量
	if (!if_have_depthstencil)
	{
		m_commandList->GetCommandList()->OMSetRenderTargets(rtv_number, rtvHandle, FALSE, NULL);
	}
	else
	{
		check_error = depth_stencil_heap->second->GetCommonDescriptorCpuOffset(depthstencil_descriptor, dsvHandle);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		m_commandList->GetCommandList()->OMSetRenderTargets(rtv_number, rtvHandle, FALSE, &dsvHandle);
	}
	delete[] rtvHandle;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::GetCommonDepthStencilBufferOffset(
	const pancy_object_id depthstencil_descriptor,
	CD3DX12_CPU_DESCRIPTOR_HANDLE &dsvHandle
)
{
	PancystarEngine::EngineFailReason check_error;
	auto depth_stencil_heap = descriptor_heap_map.find(common_descriptor_heap_depth_stencil);
	if (depth_stencil_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "haven't init common descriptor heap");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeapControl::BindCommonRenderTargetUncontiguous", error_message);
		return error_message;
	}
	check_error = depth_stencil_heap->second->GetCommonDescriptorCpuOffset(depthstencil_descriptor, dsvHandle);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}

