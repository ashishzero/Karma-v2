#include "kFilters.fx"

cbuffer constants : register(b0)
{
	uint MipLevel;
}

Texture2D<float3> TexImage : register(t0);
SamplerState Sampler : register(s0);
RWTexture2D<float3> Output : register(u0);

[numthreads(32, 16, 1)]
void Main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 pos = DTid.xy;
	
	uint w, h, mips;
	TexImage.GetDimensions(MipLevel + 1, w, h, mips);
	
	if (pos.x < w && pos.y < h)
	{
		float2 uv = (float2)pos / float2(w, h);
		float lod = (float)MipLevel / mips;
		Output[pos] = kDownsample36(TexImage, Sampler, uv, lod);
	}
}
