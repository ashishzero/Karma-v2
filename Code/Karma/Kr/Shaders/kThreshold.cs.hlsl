
// Knee curve parameters
cbuffer constants : register(b0)
{
	float3 Intensity;
	float Threshold;
}

float3 kThreshold(float3 Color)
{
	float3 Factor = float3(0.2126, 0.7152, 0.0722);
	float3 Luma = Factor * Color * Intensity;
	float Y = step(Threshold, Luma);
	return Y * Color;
}

Texture2D<float4> TexImage : register(t0);
RWTexture2D<float3> Output : register(u0);

[numthreads(32, 16, 1)]
void Main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 Pos = DTid.xy;

	uint Width, Height;
	TexImage.GetDimensions(Width, Height);
	
	if (Pos.x < Width && Pos.y < Height)
	{
		float4 Sampled = TexImage[Pos];
		float3 Color = kThreshold(Sampled.xyz);
		Output[Pos] = Color;
	}
}
