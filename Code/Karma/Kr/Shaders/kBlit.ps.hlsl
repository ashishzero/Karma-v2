#include "kBlit.fx"

Texture2D    TexImage : register(t0);
SamplerState Sampler : register(s0);

float4 Main(kVertexOutput input) : SV_Target
{
	return TexImage.Sample(Sampler, input.tex);
}
