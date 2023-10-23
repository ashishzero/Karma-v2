#include "kBlit.fx"

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

kVertexOutput Main(kVertexInput input)
{
	kVertexOutput output;
	output.pos = float4(Vertices[input.Id], 0, 1);
	output.tex = TexCoords[input.Id];
	return output;
}
