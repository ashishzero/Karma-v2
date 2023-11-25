#include "kBlit.fx"

static float2 VertexPositions[4] = { float2(-1.0, -1.0), float2(1.0, -1.0), float2(-1.0, 1.0), float2(1.0, 1.0) };

kVertexOutput Main(kVertexInput input)
{
	kVertexOutput Out;
	float2 Position = VertexPositions[input.Id];
	Out.Position = float4(Position, 0.0, 1.0);
	Out.TexCoord = Position * float2(0.5f, -0.5f) + 0.5f;
	return Out;
}
