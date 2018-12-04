cbuffer per_object : register(b0)
{
	float4x4 world_matrix;
	float4x4 WVP_matrix;
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
struct PSInput
{
	float4 position : SV_POSITION;
	float4 pos_out:POSITION;
	float4 color : COLOR;
};
PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;

	result.position = mul(position, WVP_matrix);
	result.color = color;
	result.pos_out = mul(position, world_matrix);
	result.pos_out = mul(result.pos_out, view_matrix);
	return result;
}
float4 PSMain(PSInput input) : SV_TARGET
{
	float4 color_1 = texture_test[0].Sample(g_sampler, input.color.xy);
	return color_1;
}