
Texture2D gBaseMap : register(t0);
Texture2D gEdgeMap : register(t1);

SamplerState gSamplerState : register(s0);

static const float2 gTexCoords[6] =
{
	float2(0.0f, 1.0f),
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 0.0f),
	float2(1.0f, 1.0f),
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
	VertexOut vout;
	vout.TexC = gTexCoords[vid];

	// [0,1]^2 을 NDC공간으로 사상한다.
	vout.PosH = float4(
		2.0f * vout.TexC.x - 1.0f, 
		1.0f - 2.0f * vout.TexC.y, 
		0.0f, 
		1.0f);

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 c = gBaseMap.SampleLevel(gSamplerState, pin.TexC, 0.0f);
	float4 e = gEdgeMap.SampleLevel(gSamplerState, pin.TexC, 0.0f);
	
	// 윤곽선 검출 맵을 원본 이미지에 곱한다.
	return c * e;
}