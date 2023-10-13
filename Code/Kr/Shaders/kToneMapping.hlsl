
struct kVertexOutput
{
	float2 tex : TEXCOORD;
	float4 pos : SV_Position;
};

Texture2D    TexImage : register(t0);
SamplerState Sampler : register(s0);

float4 kToneMapSdrPS(kVertexOutput input) : SV_Target
{
	return TexImage.Sample(Sampler, input.tex);
}

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

float4 kToneMapAcesApproxPS(kVertexOutput input) : SV_Target
{
	float4 sampled = TexImage.Sample(Sampler, input.tex);
	float3 mapped  = kAcesApprox(sampled.rgb);
	float4 color   = float4(mapped, 1.0);
	return color;
}
