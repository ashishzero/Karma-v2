#include "kFilters.fx"

Texture2D<float3> TexImage : register(t0);
SamplerState Sampler : register(s0);
RWTexture2D<float3> Output : register(u0);

[numthreads(32, 16, 1)]
void Main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 pos = DTid.xy;
	
	float w, h;
	Output.GetDimensions(w, h);
	
	if (pos.x < (uint)w && pos.y < (uint)h)
	{
		float2 uv = (float2) pos / float2(w - 1, h - 1);
		float3 color = kDownsampleKarisAverage(TexImage, Sampler, uv);
		Output[pos]  = color;
	}
}
