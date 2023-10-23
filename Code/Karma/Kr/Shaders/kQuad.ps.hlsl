#include "kQuad.fx"

Texture2D           SrcTexture : register(t0);
Texture2D           MaskTexture : register(t1);
SamplerState        Sampler : register(s0);

static const float3 OutLineColor = float3(1.0, 1.0, 1.0);
static const float  OutLineWidth = 0.1;

float4 Main(kVertexOutput input) : SV_Target
{
	float4 sampled = SrcTexture.Sample(Sampler, input.tex);
	float  sd_mask = MaskTexture.Sample(Sampler, input.tex).r;
	float4 masked = MaskTexture.Sample(Sampler, input.tex);
	float  aaf = fwidth(sd_mask);
	float  factor = smoothstep(0.5 - aaf, 0.5 + aaf, sd_mask);
	float3 color = lerp(OutLineColor, sampled.rgb * input.col.rgb, factor);
	float  outdist = (1.0 - OutLineWidth) * 0.5;
	float  alpha = smoothstep(outdist - aaf, outdist + aaf, sd_mask);
	return float4(color, sampled.a * input.col.a * alpha);
}
