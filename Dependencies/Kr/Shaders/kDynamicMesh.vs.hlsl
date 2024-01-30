#include "kMesh.fx"

cbuffer ProjectionViewBuffer : register(b0)
{
	row_major float4x4 ProjectionView;
}

cbuffer TransformBuffer : register(b1)
{
	row_major float4x4 Transform;
}

cbuffer SkeletonBuffer : register(b2)
{
	row_major float4x4 Bones[256];
}

kVertexOutput Main(kDynamicVertexInput In)
{
	float4x4 Frame = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	
	for (int Iter = 0; Iter < 4; ++Iter)
	{
		Frame += In.Weight[Iter] * Bones[In.Bones[Iter]];
	}
	
	float4x4 Local = Transform * Frame;
	
	float4 Vertex = mul(Local, float4(In.Position, 1.0));
	float4 Normal = mul(Local, float4(In.Normal, 0.0));
		
	kVertexOutput Out;
	Out.Position = mul(ProjectionView, Vertex);
	Out.TexCoord = In.TexCoord;
	Out.Color = In.Color;
	Out.Normal = Normal;
	
	return Out;
}
