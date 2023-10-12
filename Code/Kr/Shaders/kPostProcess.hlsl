////////////////////////////////////////////////

struct kVertexInput
{
	uint Id : SV_VertexID;
};

struct kVertexOutput
{
	float2 tex : TEXCOORD;
	float4 pos : SV_Position;
};

static const float2 Vertices[] =
{
	{ -1, -1 },
	{ -1, 1 },
	{ 1, 1 },
	{ -1, -1 },
	{ 1, 1 },
	{ 1, -1 }
};
static const float2 TexCoords[] =
{
	{ 0, 1 },
	{ 0, 0 },
	{ 1, 0 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 1 }
};

kVertexOutput RectVs(kVertexInput input)
{
	kVertexOutput output;
	output.pos = float4(Vertices[input.Id], 0, 1);
	output.tex = TexCoords[input.Id];
	return output;
}

////////////////////////////////////////////////

Texture2D TexImage : register(t0);
SamplerState Sampler : register(s0);

float4 RectPs(kVertexOutput input) : SV_Target
{
	return TexImage.Sample(Sampler, input.tex);
}

////////////////////////////////////////////////
