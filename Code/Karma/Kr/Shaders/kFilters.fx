// https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare

float3 kDownsample36(Texture2D<float3> TexImage, SamplerState Sampler, float2 TexCoord)
{
	/*
		UL -- UU -- UR
		-- M0 -- M1 --
		LL -- MM -- RR
		-- M2 -- M3 --
		DL -- DD -- DR
	*/
		
	float3 MM = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(0, 0));
		
	float3 M0 = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(-1, -1));
	float3 M1 = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(1, -1));
	float3 M2 = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(-1, 1));
	float3 M3 = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(1, 1));
		
	float3 UL = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(-2, -2));
	float3 UR = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(2, -2));
	float3 DL = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(-2, 2));
	float3 DR = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(2, 2));
		
	float3 UU = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(0, -2));
	float3 DD = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(0, 2));
	float3 LL = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(-2, 0));
	float3 RR = TexImage.SampleLevel(Sampler, TexCoord, 0, int2(2, 0));
	
	float3 FC = MM * 0.125;
	FC += (UL + UR + DL + DR) * 0.03125;
	FC += (UU + DD + LL + RR) * 0.0625;
	FC += (M0 + M1 + M2 + M3) * 0.125;

	return FC;
}

float3 kBlurUpSampleTent3x3(Texture2D<float3> TexImage, SamplerState Sampler, float2 TexCoord, float2 Radius)
{
	/*
		UL UU UR
		LL MM RR
		DL DD DR
	*/

	float3 MM = TexImage.SampleLevel(Sampler, TexCoord, 0);
	
	float3 UL = TexImage.SampleLevel(Sampler, float2(TexCoord.x - Radius.x, TexCoord.y - Radius.y), 0);
	float3 UR = TexImage.SampleLevel(Sampler, float2(TexCoord.x - Radius.x, TexCoord.y + Radius.y), 0);
	float3 DL = TexImage.SampleLevel(Sampler, float2(TexCoord.x + Radius.x, TexCoord.y - Radius.y), 0);
	float3 DR = TexImage.SampleLevel(Sampler, float2(TexCoord.x + Radius.x, TexCoord.y + Radius.y), 0);
	
	float3 UU = TexImage.SampleLevel(Sampler, float2(TexCoord.x, TexCoord.y - Radius.y), 0);
	float3 DD = TexImage.SampleLevel(Sampler, float2(TexCoord.x, TexCoord.y + Radius.y), 0);
	float3 LL = TexImage.SampleLevel(Sampler, float2(TexCoord.x - Radius.x, TexCoord.y), 0);
	float3 RR = TexImage.SampleLevel(Sampler, float2(TexCoord.x + Radius.x, TexCoord.y), 0);

	float3 FC = MM * 4.0;
	FC += (UU + DD + LL + RR) * 2.0;
	FC += (UL + UR + DL + DR);
	FC *= 1.0 / 16.0;
	
	return FC;
}
