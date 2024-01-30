struct kVertexInput
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float4 Color : COLOR;
};

struct kVertexOutput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
	float4 Color : COLOR;
};
