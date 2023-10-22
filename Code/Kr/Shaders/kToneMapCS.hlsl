
float3 kAcesApprox(float3 v)
{
	v *= 0.6;
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0, 1.0);
}

Texture2D<float4> TexImage : register(t0);
RWTexture2D<float3> Output : register(u0);

[numthreads(32, 16, 1)]
void kToneMapAcesCS(uint3 DTid : SV_DispatchThreadID)
{
	uint2 pos = DTid.xy;
	
	uint w, h;
	TexImage.GetDimensions(w, h);
	
	if (pos.x < w && pos.y < h)
	{
		float4 sampled = TexImage[pos];
		float3 mapped = kAcesApprox(sampled.xyz);
		Output[pos] = mapped;
	}
}
