#include "kMesh.fx"

cbuffer constants : register(b0)
{
	row_major float4x4 ProjectionView;
}

struct kTransform
{
	row_major float4x4 Model;
	row_major float3x3 Normal;
};

cbuffer constants : register(b1)
{
	kTransform Transform;
}

kVertexOutput Main(kVertexInput In)
{
	float4 Vertex = mul(Transform.Model, float4(In.Position, 1.0));
		
	kVertexOutput Out;
	Out.Position = mul(ProjectionView, Vertex);
	Out.TexCoord = In.TexCoord;
	Out.Color = In.Color;
	Out.Vertex = Vertex.xyz;
	Out.Normal = mul(Transform.Normal, In.Normal);
	
	
	
	return Out;
}
