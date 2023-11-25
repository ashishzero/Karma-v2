#include "kBlit.fx"

Texture2D TexImage0 : register(t0);
Texture2D TexImage1 : register(t1);
SamplerState Sampler : register(s0);

float4 Main(kVertexOutput In) : SV_Target
{
	float3 Color0 = TexImage0.Sample(Sampler, In.TexCoord);
	float3 Color1 = TexImage1.Sample(Sampler, In.TexCoord);
	float3 Color = saturate(Color0 + Color1);
	return float4(Color, 1.0);
}
