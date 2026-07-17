// =============================================================================
//  DeferredLighting.hlsl  |  Lighting pass — fullscreen quad reads G-Buffers
//  Reads RT0–RT3 + shadow map, computes Lambert diffuse + Blinn-Phong specular.
//
//  Debug view modes (DebugViewMode):
//    0 = final lit result      4 = Roughness
//    1 = Albedo                5 = AO
//    2 = Normals (decoded)     6 = Emissive
//    3 = World Position        7 = Shadow factor
// =============================================================================

// ─── Constant Buffers ────────────────────────────────────────────────────────

cbuffer CBPerFrame : register(b0)
{
    matrix View;
    matrix Projection;
    matrix LightViewProjection;
    float3 CameraPos;
    float  _pad0;
    float3 LightDir;       // Direction OF the light (toward the scene; negate for Lambert)
    float  _pad1;
    float3 LightColor;
    float  LightRange;
    float3 LightPosition;
    int    LightType;
    float4 LightPositionsRanges[8];
    float4 LightColorsTypes[8];
    float4 LightDirectionsIntensities[8];
    int    LightCount;
    float3 _pad2;
    float4 LightSpotAngles[8]; // .x = semiangulo del cono (radianes) de cada luz Spot
}

cbuffer DebugData : register(b1)
{
    int   DebugViewMode;   // Which channel to visualize (0 = final)
    float ShadowStrength;  // Shadow blend factor (1 = full shadow)
    float _dpad0;
    float _dpad1;
}

// ─── G-Buffer textures (slots t0–t3) + shadow map (t6) ───────────────────────
Texture2D GBuffer0  : register(t0); // Albedo (RGB) + Metallic (A)
Texture2D GBuffer1  : register(t1); // Encoded Normal (RGB) + Roughness (A)
Texture2D GBuffer2  : register(t2); // World Position (RGB) + AO (A)
Texture2D GBuffer3  : register(t3); // Emissive (RGB) + Alpha (A)
Texture2D ShadowMap : register(t6); // Shadow depth map (R24_UNORM)

SamplerState Sampler : register(s0);

// ─── Vertex I/O (fullscreen quad uses full SimpleVertex layout) ───────────────
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
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

// ─── Vertex Shader — pass-through (positions already in NDC) ─────────────────
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.tex = input.tex;
    return output;
}

// ─── PCF 3×3 shadow sampling ─────────────────────────────────────────────────
float ComputeShadow(float3 worldPos)
{
    float4 shadowPosH = mul(float4(worldPos, 1.0f), LightViewProjection);
    float3 proj = shadowPosH.xyz / shadowPosH.w;

    float2 uv;
    uv.x =  proj.x * 0.5f + 0.5f;
    uv.y = -proj.y * 0.5f + 0.5f;

    // Outside shadow frustum → fully lit
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || proj.z > 1.0f)
        return 1.0f;

    float depth      = proj.z - 0.003f; // shadow bias
    float texelSize  = 1.0f / 2048.0f;
    float shadow     = 0.0f;

    [unroll] for (int x = -1; x <= 1; ++x)
    [unroll] for (int y = -1; y <= 1; ++y)
    {
        float smDepth = ShadowMap.Sample(Sampler, uv + float2(x, y) * texelSize).r;
        shadow += (depth <= smDepth) ? 1.0f : 0.0f;
    }

    return shadow / 9.0f;
}

// ─── Pixel Shader ────────────────────────────────────────────────────────────
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float2 uv = input.tex;

    // Read G-Buffers
    float4 rt0 = GBuffer0.Sample(Sampler, uv);
    float4 rt1 = GBuffer1.Sample(Sampler, uv);
    float4 rt2 = GBuffer2.Sample(Sampler, uv);
    float4 rt3 = GBuffer3.Sample(Sampler, uv);

    // Unpack G-Buffer data
    float3 albedo    = rt0.rgb;
    float  metallic  = rt0.a;
    float3 N         = normalize(rt1.rgb * 2.0f - 1.0f); // decode [0,1] → [-1,1]
    float  roughness = rt1.a;
    float3 worldPos  = rt2.rgb;
    float  ao        = rt2.a;
    float3 emissive  = rt3.rgb;

    // ─── Debug views ─────────────────────────────────────────────────────────
    if (DebugViewMode == 1) return float4(albedo, 1.0f);
    if (DebugViewMode == 2) return float4(N * 0.5f + 0.5f, 1.0f);
    if (DebugViewMode == 3) return float4(frac(abs(worldPos) * 0.05f), 1.0f);
    if (DebugViewMode == 4) return float4(roughness, roughness, roughness, 1.0f);
    if (DebugViewMode == 5) return float4(ao, ao, ao, 1.0f);
    if (DebugViewMode == 6) return float4(emissive, 1.0f);
    if (DebugViewMode == 7) return float4(ComputeShadow(worldPos).xxx, 1.0f);

    // ─── Skip lighting for background pixels (worldPos ≈ 0 and normal invalid) ─
    // G-Buffer clear sets RT2 to (0,0,0,1). If all 4 comps are near default,
    // this is a background pixel — return a sky color.
    if (dot(rt0, rt0) < 0.001f)
        return float4(0.0f, 0.125f, 0.30f, 1.0f); // engine background color

    // ─── Multi-light accumulation (Directional / Point / Spot) ────────────────
    // Cada luz activa (hasta LightCount, empacadas por DeferredRenderer::writeLightToFrameBuffer)
    // aporta su propia direccion L y atenuacion: Directional no atenua; Point atenua por
    // distancia hasta 'range'; Spot ademas recorta por el cono definido en LightSpotAngles.
    float3 V = normalize(CameraPos - worldPos);
    float  shininess = max(1.0f, (1.0f - roughness) * 256.0f);

    float3 totalDiffuse  = float3(0.0f, 0.0f, 0.0f);
    float3 totalSpecular = float3(0.0f, 0.0f, 0.0f);

    int count = min(LightCount, 8);
    for (int i = 0; i < count; ++i)
    {
        float4 posRange     = LightPositionsRanges[i];
        float4 colorType    = LightColorsTypes[i];
        float4 dirIntensity = LightDirectionsIntensities[i];
        int    lightType    = (int)colorType.w;
        float3 lightColorI  = colorType.xyz; // ya incluye intensity (color * intensity)

        float3 L;
        float  attenuation = 1.0f;

        if (lightType == 0) // Directional
        {
            L = normalize(-dirIntensity.xyz);
        }
        else // Point (1) o Spot (2)
        {
            float3 toLight = posRange.xyz - worldPos;
            float  dist    = length(toLight);
            L = toLight / max(dist, 0.0001f);

            float range   = max(posRange.w, 0.001f);
            float falloff = saturate(1.0f - (dist / range));
            attenuation   = falloff * falloff;

            if (lightType == 2) // Spot: recorte adicional por el cono
            {
                float3 spotDir   = normalize(dirIntensity.xyz);
                float  cosAngle  = dot(-L, spotDir);
                float  cosCutoff = cos(LightSpotAngles[i].x);
                float  spotFactor = saturate((cosAngle - cosCutoff) / max(1.0f - cosCutoff, 0.001f));
                attenuation *= spotFactor;
            }
        }

        float NdotL = saturate(dot(N, L));
        totalDiffuse += NdotL * lightColorI * attenuation;

        float3 H     = normalize(L + V);
        float  NdotH = saturate(dot(N, H));
        totalSpecular += pow(NdotH, shininess) * lightColorI * attenuation;
    }

    totalSpecular *= (1.0f - roughness) * (1.0f - metallic);

    float3 ambient = 0.08f * albedo * ao; // termino ambiental fijo, independiente del numero de luces
    float3 diffuse = totalDiffuse * albedo;

    float shadowFactor = lerp(1.0f, ComputeShadow(worldPos), ShadowStrength);

    float3 color = ambient
                 + shadowFactor * (diffuse + totalSpecular)
                 + emissive;

    return float4(color, 1.0f);
}
