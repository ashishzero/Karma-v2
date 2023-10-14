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

Texture2D    SrcTexture : register(t0);
Texture2D    MaskTexture : register(t1);
SamplerState Sampler : register(s0);

////////////////////////////////////////////////

kVertexOutput kQuadVS(kVertexInput vertex)
{
	kVertexOutput ouput;
	ouput.pos = mul(Transform, float4(vertex.pos, 1.0f));
	ouput.tex = vertex.tex;
	ouput.col = vertex.col;
	return ouput;
}

////////////////////////////////////////////////

float4 kQuadPS(kVertexOutput input) : SV_Target
{
	float4 sampled = SrcTexture.Sample(Sampler, input.tex);
	float  sd_mask = MaskTexture.Sample(Sampler, input.tex).r;
	float4 masked  = MaskTexture.Sample(Sampler, input.tex);
	float  aaf     = fwidth(sd_mask);
	float  alpha   = smoothstep(0.5 - aaf, 0.5 + aaf, sd_mask);
	float4 color   = float4(1.0, 1.0, 1.0, alpha) * sampled * input.col;
	return color;
}

////////////////////////////////////////////////
