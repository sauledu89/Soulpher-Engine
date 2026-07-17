// =============================================================================
//  Gizmo.fx  |  Shader unlit para los gizmos de transformación del editor.
//  Sin iluminación: el color final es un color plano por eje (CBGizmoObject.Color),
//  usado para resaltar hover/drag. InputLayout: solo POSITION.
// =============================================================================

cbuffer CBGizmoFrame : register(b0)
{
    matrix ViewProj;
}

cbuffer CBGizmoObject : register(b1)
{
    matrix World;
    float4 Color;
}

struct VS_INPUT
{
    float3 pos : POSITION;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    float4 worldPos = mul(float4(input.pos, 1.0f), World);
    output.pos = mul(worldPos, ViewProj);
    return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    return Color;
}
