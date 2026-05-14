// =============================================================================
//  ShadowDepth.hlsl  |  Shadow depth pass — vertex only, no pixel output
//  Transforms each vertex to light clip space for shadow map generation.
// =============================================================================

cbuffer CBPerFrame : register(b0)
{
    matrix View;
    matrix Projection;
    matrix LightViewProjection;
    float3 CameraPos;
    float  _pad0;
    float3 LightDir;
    float  _pad1;
    float3 LightColor;
    float  _pad2;
}

cbuffer CBPerObject : register(b1)
{
    matrix World;
}

// Only POSITION is needed; the rest of SimpleVertex is ignored in this pass.
float4 VS(float3 pos : POSITION) : SV_POSITION
{
    float4 worldPos = mul(float4(pos, 1.0f), World);
    return mul(worldPos, LightViewProjection);
}
