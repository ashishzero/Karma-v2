#include "kQuad.fx"

cbuffer constants : register(b0)
{
	row_major float4x4 Transform;
}

kVertexOutput Main(kVertexInput In)
{
	kVertexOutput Out;
	Out.Position = mul(Transform, float4(In.Position, 1.0f));
	Out.TexCoord = In.TexCoord;
	Out.Color = In.Color;
	return Out;
}
