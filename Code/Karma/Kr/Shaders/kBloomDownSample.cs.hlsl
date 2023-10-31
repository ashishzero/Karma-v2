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
		float2 uv = ((float2) pos + float2(0.5, 0.5)) / float2(w, h);
		float3 color = kDownsample36(TexImage, Sampler, uv);
		Output[pos]  = color;
	}
}
