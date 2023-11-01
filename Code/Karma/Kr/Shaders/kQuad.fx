struct kVertexInput
{
	float3 Position : POS;
	float2 TexCoord : TEX;
	float4 Color : COL;
};

struct kVertexOutput
{
	float4 Position : SV_Position;
	float2 TexCoord : TEX;
	float4 Color : COL;
};
