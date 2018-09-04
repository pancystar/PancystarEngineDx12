#include"PancyResourceBasic.h"
using namespace PancystarEngine;
//»ù´¡×ÊÔ´
PancyBasicVirtualResource::PancyBasicVirtualResource(const uint32_t &resource_id_in)
{
	resource_id = resource_id_in;
	reference_count.store(0);
}
PancyBasicVirtualResource::~PancyBasicVirtualResource()
{
}
void PancyBasicVirtualResource::AddReference()
{
	reference_count.fetch_add(1);
}
void PancyBasicVirtualResource::DeleteReference()
{
	if (reference_count > 0)
	{
		reference_count.fetch_sub(1);
	}
	else
	{
		reference_count.store(0);
	}
}
PancystarEngine::EngineFailReason PancyBasicVirtualResource::Create(std::string desc_file_in)
{
	auto check_error = InitResource(desc_file_in);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
