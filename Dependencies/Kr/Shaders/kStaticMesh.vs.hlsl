#include "kMesh.fx"

cbuffer ProjectViewBuffer : register(b0)
{
	row_major float4x4 ProjectionView;
}

cbuffer TransformBuffer : register(b1)
{
	row_major float4x4 Transform;
}

kVertexOutput Main(kVertexInput In)
{
	float4 Vertex = mul(Transform, float4(In.Position, 1.0));
	float4 Normal = mul(Transform, float4(In.Normal, 0.0));
		
	kVertexOutput Out;
	Out.Position = mul(ProjectionView, Vertex);
	Out.TexCoord = In.TexCoord;
	Out.Color = In.Color;
	Out.Normal = Normal;

	
	return Out;
}
