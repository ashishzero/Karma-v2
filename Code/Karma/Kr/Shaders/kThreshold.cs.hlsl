
// Knee curve parameters
cbuffer constants : register(b0)
{
	float3 Intensity;
	float  Threshold;
}

float3 kThreshold(float3 color)
{
	float3 factor = float3(0.2126, 0.7152, 0.0722);
	float3 luma = factor * color * Intensity;
	float x = step(Threshold, luma);
	return x * color;
}

Texture2D<float4> TexImage : register(t0);
RWTexture2D<float3> Output : register(u0);

[numthreads(32, 16, 1)]
void Main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 pos = DTid.xy;

	uint w, h;
	TexImage.GetDimensions(w, h);
	
	if (pos.x < w && pos.y < h)
	{
		float4 sampled = TexImage[pos];
		float3 color = kThreshold(sampled.xyz);
		Output[pos] = color;
	}
}
