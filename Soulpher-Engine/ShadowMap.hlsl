// =============================================================================
//  ShadowMap.hlsl  |  Shadow depth pass for DeferredRenderer
//  Transforms each vertex to light-clip space. No pixel output.
//  InputLayout must match SimpleVertex: POSITION NORMAL TANGENT BITANGENT TEXCOORD
// =============================================================================

cbuffer CBPerFrame : register(b0)
{
    matrix View;
    matrix Projection;
    matrix LightViewProjection;
    // Remaining CBPerFrame fields not needed for shadow pass
}

cbuffer CBPerObject : register(b1)
{
    matrix World;
}

struct VS_INPUT
{
    float3 pos       : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float3 bitangent : BITANGENT;
    float2 tex       : TEXCOORD;
};

float4 VS(VS_INPUT input) : SV_POSITION
{
    float4 worldPos = mul(float4(input.pos, 1.0f), World);
    return mul(worldPos, LightViewProjection);
}

// Required by ShaderProgram::init() which always compiles VS+PS.
// renderShadowPass() immediately overrides this with PSSetShader(nullptr) after binding.
float4 PS() : SV_TARGET
{
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}
