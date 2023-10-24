#include "kBlit.fx"

kVertexOutput Main(kVertexInput input)
{
	kVertexOutput output;
	output.tex = float2((input.Id << 1) & 2, input.Id & 2);
	output.pos = float4(output.tex * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	return output;
}
