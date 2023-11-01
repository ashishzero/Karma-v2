#include "kBlit.fx"

float3 kAcesApprox(float3 v)
{
	v *= 0.6;
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0, 1.0);
}

Texture2D TexImage : register(t0);
SamplerState Sampler : register(s0);

cbuffer constants : register(b0)
{
	float3 Intensity;
}

float4 Main(kVertexOutput In) : SV_Target
{
	float4 Color = TexImage.Sample(Sampler, In.TexCoord);
	float3 Output = kAcesApprox(Intensity * Color.rgb);
	return float4(Output, 1);
}
