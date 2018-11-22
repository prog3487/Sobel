
Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

// RGB 값에서 휘도(밝기)를 근사한다.
float CalcLuminance(float3 color)
{
	return dot(color, float3(0.299f, 0.587f, 0.114f));
}


[numthreads(16, 16, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
	// 이 픽셀 주변의 이웃 픽셀들을 추출한다.
	float4 c[3][3];
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			int2 xy = DTid.xy + int2(j - 1, i - 1);
			c[i][j] = gInput[xy];
		}
	}

	// 각 색상 채널에 대해, 소벨 커널을 이용해서 x,y의 편미분계수를 추정한다.
	float4 Gx = -1.0f*c[0][0] - 2.0f*c[1][0] - 1.0f*c[2][0] + 1.0f*c[0][2] + 2.0f*c[1][2] + 1.0f*c[2][2];
	float4 Gy = -1.0f*c[2][0] - 2.0f*c[2][1] - 1.0f*c[2][1] + 1.0f*c[0][0] + 2.0f*c[0][1] + 1.0f*c[0][2];

	// 이제 Gx, Gy가 이 픽셀의 기울기이다. 각 색상 채널에 대해 변화율이 최대가 되는 크기를 구한다.
	float4 mag = sqrt(Gx * Gx + Gy * Gy);

	// 윤곽선 픽셀은 검게, 윤곽선이 아닌 픽셀은 희게 만든다.
	mag = 1.0f - saturate(CalcLuminance(mag.rgb));

	gOutput[DTid.xy] = mag;
}