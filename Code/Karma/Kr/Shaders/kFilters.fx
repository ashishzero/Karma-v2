// https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare

float3 kDownsample36(Texture2D<float3> TexImage, SamplerState Sampler, float2 uv, float lod)
{
	/*
		UL -- UU -- UR
		-- M0 -- M1 --
		LL -- MM -- RR
		-- M2 -- M3 --
		DL -- DD -- DR
	*/
		
	float3 MM = TexImage.SampleLevel(Sampler, uv, lod, int2(0, 0));
		
	float3 M0 = TexImage.SampleLevel(Sampler, uv, lod, int2(-1, -1));
	float3 M1 = TexImage.SampleLevel(Sampler, uv, lod, int2(1, -1));
	float3 M2 = TexImage.SampleLevel(Sampler, uv, lod, int2(-1, 1));
	float3 M3 = TexImage.SampleLevel(Sampler, uv, lod, int2(1, 1));
		
	float3 UL = TexImage.SampleLevel(Sampler, uv, lod, int2(-2, -2));
	float3 UR = TexImage.SampleLevel(Sampler, uv, lod, int2(2, -2));
	float3 DL = TexImage.SampleLevel(Sampler, uv, lod, int2(-2, 2));
	float3 DR = TexImage.SampleLevel(Sampler, uv, lod, int2(2, 2));
		
	float3 UU = TexImage.SampleLevel(Sampler, uv, lod, int2(0, -2));
	float3 DD = TexImage.SampleLevel(Sampler, uv, lod, int2(0, 2));
	float3 LL = TexImage.SampleLevel(Sampler, uv, lod, int2(-2, 0));
	float3 RR = TexImage.SampleLevel(Sampler, uv, lod, int2(2, 0));
		
	float3 C0 = (UL + UU + LL + MM) * 0.25;
	float3 C1 = (UU + UR + MM + RR) * 0.25;
	float3 C2 = (LL + MM + DL + DD) * 0.25;
	float3 C3 = (MM + RR + DD + DR) * 0.25;
	float3 CC = (M0 + M1 + M2 + M3) * 0.25;
		
	float3 FC = 0.5 * CC;
	FC += 0.125 * C0;
	FC += 0.125 * C1;
	FC += 0.125 * C2;
	FC += 0.125 * C3;
	
	return FC;
}
