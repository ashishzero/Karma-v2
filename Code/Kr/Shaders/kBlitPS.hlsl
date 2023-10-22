
struct kVertexOutput
{
	float2 tex : TEXCOORD;
	float4 pos : SV_Position;
};

Texture2D    TexImage : register(t0);
SamplerState Sampler : register(s0);

float4 kBlitPS(kVertexOutput input) : SV_Target
{
	return TexImage.Sample(Sampler, input.tex);
}
