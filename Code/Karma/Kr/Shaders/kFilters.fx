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

float3 kPowFloat3(float3 v, float p)
{
	return float3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

float3 kLinearToSRGB(float3 v)
{
	float gamma = 2.2;
	return kPowFloat3(v, 1.0 / gamma);
}

float kLuminance(float3 color)
{
	float3 factor = float3(0.2126, 0.7152, 0.0722);
	return dot(factor, color);
}

float kKarisAverage(float3 color)
{
	float luma = kLuminance(kLinearToSRGB(color)) * 0.25f;
	return 1.0f / (1.0f + luma);
}

float3 kDownsampleKarisAverage(Texture2D<float3> TexImage, SamplerState Sampler, float2 TexCoord)
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
	
	float3 C0 = (UL + UU + LL + MM) * 0.125 * 0.25;
	float3 C1 = (UU + UR + MM + RR) * 0.125 * 0.25;
	float3 C2 = (LL + MM + DL + DD) * 0.125 * 0.25;
	float3 C3 = (MM + RR + DD + DR) * 0.125 * 0.25;
	float3 C4 = (M0 + M1 + M2 + M3) * 0.5 * 0.25;

	float3 FC = C0 * kKarisAverage(C0);
	FC += C1 * kKarisAverage(C1);
	FC += C2 * kKarisAverage(C2);
	FC += C3 * kKarisAverage(C3);
	FC += C4 * kKarisAverage(C4);

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
