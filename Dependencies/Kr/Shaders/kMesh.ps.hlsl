#include "kMesh.fx"

struct kMaterial
{
	   float4 Color;
};

cbuffer constants : register(b0)
{
	kMaterial Material;
}

Texture2D Diffuse : register(t0);
SamplerState Sampler : register(s0);

float CalcDirLight(float3 Direction, float3 Normal)
{
	float3 LightDir = normalize(-Direction);
	float Diffuse = max(dot(Normal, LightDir), 0);
	return 0.1 + Diffuse;
}

float4 Main(kVertexOutput In) : SV_Target
{
	float4 shade = Diffuse.Sample(Sampler, In.TexCoord) * In.Color;
	float4 color = shade * Material.Color;
	
	color.xyz *= CalcDirLight(float3(0, 0, 1), normalize(In.Normal));
	//color.xyz = In.Normal;
	return color;
}
