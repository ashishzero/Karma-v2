////////////////////////////////////////////////

struct kVertexInput
{
	float3 pos : POS;
	float2 tex : TEX;
	float4 col : COL;
};

struct kVertexOutput
{
	float2 tex : TEX;
	float4 col : COL;
	float4 pos : SV_Position;
};

cbuffer constants : register(b0)
{
	row_major float4x4 Transform;
}

Texture2D	 SrcTexture : register(t0);
Texture2D	 MaskTexture : register(t1);
SamplerState Sampler : register(s0);

////////////////////////////////////////////////

kVertexOutput QuadVsMain(kVertexInput vertex)
{
	kVertexOutput ouput;
	ouput.pos = mul(Transform, float4(vertex.pos, 1.0f));
	ouput.tex = vertex.tex;
	ouput.col = vertex.col;
	return ouput;
}

////////////////////////////////////////////////

float4 QuadPsMain(kVertexOutput input) : SV_Target
{
	float4 sampled = SrcTexture.Sample(Sampler, input.tex);
	float4 masked  = MaskTexture.Sample(Sampler, input.tex);
	float4 color   = sampled * masked * input.col;
	return color;
}

////////////////////////////////////////////////

float Median(float r, float g, float b)
{
	return max(min(r, g), min(max(r, g), b));
}

float4 SignedDistPsMain(kVertexOutput input) : SV_Target
{
	float3 sampled = SrcTexture.Sample(Sampler, input.tex).rgb;
	float sd       = Median(sampled.r, sampled.g, sampled.b);
	float4 masked  = MaskTexture.Sample(Sampler, input.tex);
	float  aaf	   = fwidth(sd);
	float  alpha   = smoothstep(0.5 - aaf, 0.5 + aaf, sd);
	float4 color   = float4(1.0, 1.0, 1.0, alpha) * masked * input.col;
	return color;
}

////////////////////////////////////////////////
