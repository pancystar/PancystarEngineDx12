struct instance_data 
{
	float4x4 world_matrix;
	uint4 animation_index;
	//float4x4 normal_matrix;
};
cbuffer per_instance : register(b0) 
{
	instance_data _Instances[100];
}
cbuffer per_frame : register(b1)
{
	float4x4 view_matrix;
	float4x4 projectmatrix;
	float4x4 invview_matrix;
	float4 view_position;
}
texture2D texture_test[] : register(t0);
//texture2D texture_test : register(t0);
SamplerState g_sampler : register(s0);
struct VSInput 
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float3 tangent  : TANGENT;
	uint4  tex_id   : TEXID;
	float4 tex_uv   : TEXUV;
};
struct PSInput
{
	float4 position : SV_POSITION;
	float4 pos_out  :POSITION;
	float4 color    : COLOR;
	uint4  tex_id   : TEXID;
	float4 tex_uv   : TEXUV;
};
PSInput VSMain(VSInput vinput)
{
	PSInput result;
	result.pos_out = mul(float4(vinput.position, 1.0), _Instances[0].world_matrix);
	result.position = mul(result.pos_out, view_matrix);
	result.position = mul(result.position, projectmatrix);
	result.color = float4(vinput.normal,1.0f);
	
	result.pos_out = mul(result.pos_out, view_matrix);
	result.tex_id = vinput.tex_id;
	result.tex_uv.xy =vinput.tex_uv.xy;
	result.tex_uv.zw =vinput.tex_uv.zw;
	return result;
}
float4 PSMain(PSInput input) : SV_TARGET
{
	float4 color_1 = texture_test[0].Sample(g_sampler, input.tex_uv.xy);
	return color_1;
}