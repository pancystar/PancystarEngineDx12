cbuffer per_object
{
	float4x4 world_matrix;
	float4x4 view_matrix;
	float4x4 projectmatrix;
	float4x4 WVP_matrix;
	float4x4 invview_matrix;
	float4 view_position;
}
struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};
PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;

	result.position = position;
	result.color = color;

	return result;
}
float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
}