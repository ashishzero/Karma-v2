
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
		Output[Pos] = TexImage[Pos].xyz;
	}
}
