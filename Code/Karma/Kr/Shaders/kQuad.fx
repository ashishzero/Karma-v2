struct kVertexInput
{
	float3 pos : POS;
	float2 tex : TEX;
	float4 col : COL;
};

struct kVertexOutput
{
	float2 tex : TEX;
	float4 col : COL;
	float4 pos : SV_Position;
};
