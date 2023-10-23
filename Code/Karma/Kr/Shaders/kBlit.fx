struct kVertexInput
{
	uint Id : SV_VertexID;
};

struct kVertexOutput
{
	float2 tex : TEXCOORD;
	float4 pos : SV_Position;
};
