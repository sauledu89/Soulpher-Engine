// =============================================================================
//  DeferredGBuffer.hlsl  |  Geometry pass — writes surface data to 4 MRTs
//  RT0: Albedo (RGB) + Metallic (A)        — R8G8B8A8_UNORM
//  RT1: Normal (RGB, encoded [0,1]) + Roughness (A) — R16G16B16A16_FLOAT
//  RT2: World Position (RGB) + AO (A)      — R32G32B32A32_FLOAT
//  RT3: Emissive (RGB) + Alpha (A)         — R16G16B16A16_FLOAT
// =============================================================================

// ─── Constant Buffers ────────────────────────────────────────────────────────

cbuffer CBPerFrame : register(b0)
{
    matrix View;
    matrix Projection;
    // Remaining CBPerFrame fields not used in geometry pass
}

cbuffer CBPerObject : register(b1)
{
    matrix World;
}

cbuffer CBPerMaterial : register(b2)
{
    float4 BaseColor;
    float  Metallic;
    float  Roughness;
    float  AO;
    float  NormalScale;
    float  EmissiveStrength;
    float  AlphaCutoff;
    float  _pad0;
    float  _pad1;
    float  _pad2;
    float  _pad3;
    float  _pad4;
    float  _pad5;
}

// ─── Textures (t0=albedo, t1=normal, t2=metallic, t3=roughness, t4=AO) ───────
// Todas opcionales: si el MaterialInstance no trae la textura, DeferredRenderer
// enlaza un default neutro (blanco para albedo/metallic/roughness/AO, normal
// plana para t1) — ver DeferredRenderer::renderGeometryObject.
Texture2D AlbedoTexture    : register(t0);
Texture2D NormalTexture    : register(t1);
Texture2D MetallicTexture  : register(t2);
Texture2D RoughnessTexture : register(t3);
Texture2D AOTexture        : register(t4);
SamplerState Sampler       : register(s0);

// ─── Vertex I/O ──────────────────────────────────────────────────────────────
struct VS_INPUT
{
    float3 pos       : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float3 bitangent : BITANGENT;
    float2 tex       : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos       : SV_POSITION;
    float3 worldPos  : TEXCOORD0;
    float3 normal    : TEXCOORD1;
    float2 tex       : TEXCOORD2;
    float3 tangent   : TEXCOORD3;
    float3 bitangent : TEXCOORD4;
};

// ─── G-Buffer output structure (4 simultaneous render targets) ───────────────
struct PS_OUTPUT
{
    float4 RT0 : SV_TARGET0; // Albedo (RGB) + Metallic (A)
    float4 RT1 : SV_TARGET1; // Encoded Normal (RGB) + Roughness (A)
    float4 RT2 : SV_TARGET2; // World Position (RGB) + AO (A)
    float4 RT3 : SV_TARGET3; // Emissive (RGB) + Alpha (A)
};

// ─── Vertex Shader ───────────────────────────────────────────────────────────
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 worldPos4 = mul(float4(input.pos, 1.0f), World);
    output.worldPos  = worldPos4.xyz;
    output.pos       = mul(mul(worldPos4, View), Projection);

    // Normal/tangent/bitangent a espacio de mundo (asume escala uniforme, igual que la normal).
    output.normal    = normalize(mul(input.normal,    (float3x3)World));
    output.tangent   = normalize(mul(input.tangent,   (float3x3)World));
    output.bitangent = normalize(mul(input.bitangent, (float3x3)World));

    output.tex = input.tex;
    return output;
}

// ─── Pixel Shader ────────────────────────────────────────────────────────────
PS_OUTPUT PS(VS_OUTPUT input)
{
    // Sample albedo; apply material base color tint
    float4 albedo = AlbedoTexture.Sample(Sampler, input.tex) * BaseColor;

    // Alpha cutoff for Masked materials (clip discards the pixel)
    clip(albedo.a - AlphaCutoff);

    // Geometric surface basis (TBN), re-ortogonalizada por si la interpolación las desalineó.
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent - N * dot(input.tangent, N));
    float3 B = normalize(input.bitangent);

    // Normal map: [0,1] -> [-1,1], atenuada por NormalScale (0 = normal geométrica pura).
    float3 tangentNormal = NormalTexture.Sample(Sampler, input.tex).rgb * 2.0f - 1.0f;
    tangentNormal.xy *= NormalScale;
    tangentNormal = normalize(tangentNormal);
    float3x3 TBN = float3x3(T, B, N);
    N = normalize(mul(tangentNormal, TBN));

    // Metallic/Roughness/AO: textura (canal R) multiplicada por el escalar de CBPerMaterial.
    // Con el default blanco (sin mapa), texture.r=1 y el resultado es el escalar tal cual.
    float metallic  = MetallicTexture.Sample(Sampler, input.tex).r  * Metallic;
    float roughness = RoughnessTexture.Sample(Sampler, input.tex).r * Roughness;
    float ao        = AOTexture.Sample(Sampler, input.tex).r        * AO;

    // Encode normal from [-1,1] to [0,1] for G-Buffer storage
    float3 encodedNormal = N * 0.5f + 0.5f;

    // No emissive texture — use strength from CB
    float3 emissive = albedo.rgb * EmissiveStrength;

    PS_OUTPUT output;
    output.RT0 = float4(albedo.rgb,    metallic);       // Albedo + Metallic
    output.RT1 = float4(encodedNormal, roughness);      // Normal + Roughness
    output.RT2 = float4(input.worldPos, ao);            // WorldPos + AO
    output.RT3 = float4(emissive, albedo.a);            // Emissive + Alpha

    return output;
}
