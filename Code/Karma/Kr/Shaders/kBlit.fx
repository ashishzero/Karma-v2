struct kVertexInput
{
	uint Id : SV_VertexID;
};

struct kVertexOutput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEXCOORD;
};
