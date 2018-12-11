cbuffer per_object : register(b0)
{
	float4x4 world_matrix;
	float4x4 WVP_matrix;
	float4x4 UV_matrix;
}
cbuffer per_frame : register(b1)
{
	float4x4 view_matrix;
	float4x4 projectmatrix;
	float4x4 invview_matrix;
	float4 view_position;
}
TextureCube environment_IBL_spec    : register(t0);
TextureCube environment_IBL_diffuse : register(t1);
texture2D   environment_brdf        : register(t2);
texture2D   texture_model[]         : register(t3);
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
	result.position = mul(float4(vinput.position, 1.0f), WVP_matrix);
	result.color = float4(vinput.normal, 1.0f);
	result.pos_out = mul(float4(vinput.position, 1.0), world_matrix);
	result.pos_out = mul(result.pos_out, view_matrix);
	result.tex_id = vinput.tex_id;
	result.tex_uv.xy = mul(float4(vinput.tex_uv.xy, 0, 0), UV_matrix).xy;
	result.tex_uv.zw = mul(float4(vinput.tex_uv.zw, 0, 0), UV_matrix).zw;
	return result;
}
float4 Fresnel_Schlick(float4 specularColor, float3 h, float3 v)
{
	return (specularColor + (1.0f - specularColor) * pow((1.0f - saturate(dot(v, h))), 5));
}
float4 Fresnel_CookTorrance(float4 specularColor, float3 h, float3 v)
{
	float3 n = (1.0f + sqrt(specularColor)) / (1.0f - sqrt(specularColor));
	float c = saturate(dot(v, h));
	float3 g = sqrt(n * n + c * c - 1.0f);

	float3 part1 = (g - c) / (g + c);
	float3 part2 = ((g + c) * c - 1.0f) / ((g - c) * c + 1.0f);

	return max(0.0f.xxx, 0.5f * part1 * part1 * (1 + part2 * part2));
}
void count_pbr_reflect(
	float4 tex_albedo_in,
	float  tex_matallic,
	float  tex_roughness,
	float3 light_dir_in,
	float3 normal,
	float3 direction_view,
	float diffuse_angle,
	out float4 diffuse_out,
	out float4 specular_out
)
{
	float4 F0 = lerp(0.04, tex_albedo_in, tex_matallic);
	float3 h_vec = normalize((light_dir_in + direction_view) / 2.0f);
	diffuse_out = tex_albedo_in * (1 - tex_matallic);

	float pi = 3.141592653;
	float view_angle = dot(direction_view, normal);//视线夹角
	float cos_vh = dot(direction_view, h_vec);
	float4 fresnel = F0 + (float4(1.0f, 1.0f, 1.0f, 1.0f) - F0)*(1.0f - pow(cos_vh, 5.0f));//菲涅尔项;
	//float4 fresnel = F0 + (float4(1.0f, 1.0f, 1.0f, 1.0f) - F0) * pow(2, ((-5.55473*cos_vh - 6.98316)*cos_vh));
	//NDF法线扰乱项
	float alpha = tex_roughness * tex_roughness;
	float nh_mul = dot(normal, h_vec);
	float divide_ndf1 = nh_mul * nh_mul * (alpha * alpha - 1.0f) + 1.0f;
	float divide_ndf2 = pi * divide_ndf1 *divide_ndf1;
	float ndf = (alpha*alpha) / divide_ndf2;
	//GGX遮挡项

	float ggx_k = (tex_roughness + 1.0f) * (tex_roughness + 1.0f) / 8.0f;
	float ggx_v = view_angle / (view_angle*(1 - ggx_k) + ggx_k);
	float ggx_l = diffuse_angle / (diffuse_angle*(1 - ggx_k) + ggx_k);
	float ggx = ggx_v * ggx_l;
	float3 v = reflect(light_dir_in, normal);
	float blin_phong = pow(max(dot(v, direction_view), 0.0f), 10);
	//最终的镜面反射项
	specular_out = (fresnel * ndf * ggx) / (4 * view_angle * diffuse_angle);
}