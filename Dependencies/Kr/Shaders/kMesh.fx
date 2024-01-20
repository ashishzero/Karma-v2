struct kVertexInput
{
	float3 Position : POS;
	float3 Normal   : NOR;
	float2 TexCoord : TEX;
	float4 Color    : COL;
};

struct kVertexOutput
{
	float4 Position : SV_Position;
	float4 Color : COL;
	float3 Normal: NOR;
	float3 Vertex: VTX;
	float2 TexCoord : TEX;
};
