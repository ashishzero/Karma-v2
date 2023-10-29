
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
		Output[pos] = TexImage[pos].xyz;
	}
}
