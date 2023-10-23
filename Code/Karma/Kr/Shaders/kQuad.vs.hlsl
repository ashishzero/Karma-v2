#include "kQuad.fx"

cbuffer constants : register(b0)
{
	row_major float4x4 Transform;
}

kVertexOutput Main(kVertexInput vertex)
{
	kVertexOutput ouput;
	ouput.pos = mul(Transform, float4(vertex.pos, 1.0f));
	ouput.tex = vertex.tex;
	ouput.col = vertex.col;
	return ouput;
}
