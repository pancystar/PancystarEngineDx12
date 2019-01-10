//³£Á¿»º³åÇø
cbuffer per_object : register(b0)
{
	float4x4 world_matrix;
	float4x4 WVP_matrix;
	float4x4 UV_matrix;
	float4x4 normal_matrix;
	float4x4 bone_world_matrix;
}
cbuffer per_frame : register(b1)
{
	float4x4 view_matrix;
	float4x4 projectmatrix;
	float4x4 invview_matrix;
	float4 view_position;
}
struct VSInput
{
	float3 position : POSITION;
};
struct PSInput
{
	float4 position : SV_POSITION;
	float4 pos_out  :POSITION;
};
PSInput VSMain(VSInput vinput)
{
	PSInput result;
	float3 pos_animation = mul(float4(vinput.position, 1.0f), bone_world_matrix);
	result.position = mul(float4(pos_animation, 1.0f), WVP_matrix);
	result.pos_out = mul(float4(vinput.position, 1.0), world_matrix);
	return result;
}
float4 PSMain(PSInput pin) : SV_TARGET
{
	return float4(1.0f,0,0,1.0f);
}