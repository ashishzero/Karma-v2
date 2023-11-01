#include "kQuad.fx"

Texture2D SrcTexture : register(t0);
Texture2D MaskTexture : register(t1);
SamplerState Sampler : register(s0);

cbuffer constants : register(b0)
{
	float4 OutLineStyle;
}

float4 ApplyMaskSDF(float2 TexCoord, float4 ForeGroundColor, float3 BackGroundColor, float Width)
{
	float Mask = MaskTexture.Sample(Sampler, TexCoord).r;
	float Delta = fwidth(Mask);
	float Factor = smoothstep(0.5 - Delta, 0.5 + Delta, Mask);
	float3 Color = lerp(BackGroundColor, ForeGroundColor.rgb, Factor);
	float OutDist = (1.0 - Width) * 0.5;
	float Alpha = smoothstep(OutDist - Delta, OutDist + Delta, Mask);
	return float4(Color, ForeGroundColor.a * Alpha);
}

float4 Main(kVertexOutput In) : SV_Target
{
	float4 Shade = SrcTexture.Sample(Sampler, In.TexCoord) * In.Color;
	float4 Color = ApplyMaskSDF(In.TexCoord, Shade, OutLineStyle.rgb, OutLineStyle.w);
	return Color;
}
