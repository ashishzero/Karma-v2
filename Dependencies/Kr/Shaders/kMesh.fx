struct kVertexInput
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
	float4 Color : COLOR;
};

struct kDynamicVertexInput
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
	float4 Color : COLOR;
	float4 Weight : WEIGHTS;
	uint4 Bones : BONES;
};

struct kVertexOutput
{
	float4 Position : SV_Position;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
};
