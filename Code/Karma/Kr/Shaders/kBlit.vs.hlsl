#include "kBlit.fx"

static float2 VertexPositions[4] = { float2(-1.0, -1.0), float2(1.0, -1.0), float2(-1.0, 1.0), float2(1.0, 1.0) };

kVertexOutput Main(kVertexInput input)
{
	kVertexOutput output;
	float2 position = VertexPositions[input.Id];
	output.pos = float4(position, 0.0, 1.0);
	output.tex = position * float2(0.5f, -0.5f) + 0.5f;
	return output;
}
