#include "kFilters.fx"

cbuffer constants : register(b0)
{
	float2 FilterRadius;
}

Texture2D<float3> TexImage : register(t0);
Texture2D<float3> BlurImage : register(t1);
SamplerState Sampler : register(s0);
RWTexture2D<float3> Output : register(u0);

[numthreads(32, 16, 1)]
void Main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 Pos = DTid.xy;
	
	float Width, Height;
	Output.GetDimensions(Width, Height);
	
	if (Pos.x < (uint) Width && Pos.y < (uint) Height)
	{
		float2 TexCoord = ((float2) Pos + float2(0.5, 0.5)) / float2(Width, Height);
		Output[Pos] = TexImage[Pos] + kBlurUpSampleTent3x3(BlurImage, Sampler, TexCoord, FilterRadius);
	}
}
