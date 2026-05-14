// =============================================================================
//  Soulpher-Engine.fx  |  Forward Rendering Shader
//  Vertex format: POSITION  NORMAL  TANGENT  BITANGENT  TEXCOORD
// =============================================================================

// ---------------------------------------------------------------------------
//  Recursos
// ---------------------------------------------------------------------------
Texture2D    txDiffuse : register(t0); // Albedo / textura difusa
Texture2D    txShadow  : register(t6); // Shadow map (R24_UNORM_X8_TYPELESS)
SamplerState samLinear : register(s0); // Sampler bilineal

// ---------------------------------------------------------------------------
//  Constant Buffers  (deben coincidir byte a byte con C++ CBPerFrame/CBPerObject/CBPerMaterial)
// ---------------------------------------------------------------------------

// b0 — datos por frame: camara + luz
cbuffer CBPerFrame : register(b0)
{
    matrix View;
    matrix Projection;
    matrix LightViewProjection; // Para shadow map (Fase 6)
    float3 CameraPos;
    float  _pad0;
    float3 LightDir;            // Direccion DE la luz (apunta hacia la escena)
    float  _pad1;
    float3 LightColor;
    float  ShadowBias;
}

// b1 — datos por objeto: transformacion de mundo
cbuffer CBPerObject : register(b1)
{
    matrix World;
}

// b2 — datos por material: color base y parametros PBR (Fase 6)
cbuffer CBPerMaterial : register(b2)
{
    float4 BaseColor;
    float  Metallic;
    float  Roughness;
    float  AO;
    float  NormalScale;
    float  EmissiveStrength;
    float  AlphaCutoff;
    float  _mpad0;
    float  _mpad1;
}

// ---------------------------------------------------------------------------
//  Estructuras de pipeline
// ---------------------------------------------------------------------------

struct VS_INPUT
{
    float3 Pos       : POSITION;
    float3 Normal    : NORMAL;
    float3 Tangent   : TANGENT;
    float3 Bitangent : BITANGENT;
    float2 Tex       : TEXCOORD0;
};

struct PS_INPUT
{
    float4 ClipPos    : SV_POSITION;
    float2 Tex        : TEXCOORD0;
    float3 WorldPos   : TEXCOORD1;  // Posicion en espacio mundo (para especular)
    float3 WorldNormal: TEXCOORD2;  // Normal en espacio mundo
    float4 ShadowPosH : TEXCOORD3;  // Posicion en espacio de la luz (sin dividir por w)
};

// ---------------------------------------------------------------------------
//  Vertex Shader principal
// ---------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;

    float4 worldPos = mul(float4(input.Pos, 1.0f), World);
    output.ClipPos    = mul(mul(worldPos, View), Projection);
    output.WorldPos   = worldPos.xyz;

    // Normal al espacio mundo. Para modelos con escala uniforme, mul con World
    // es correcto despues de normalizar. En Fase 6 usaremos InvTransposeWorld.
    output.WorldNormal = normalize(mul(input.Normal, (float3x3)World));

    output.Tex        = input.Tex;
    output.ShadowPosH = mul(worldPos, LightViewProjection);
    return output;
}

// ---------------------------------------------------------------------------
//  Funcion auxiliar: PCF 3x3 sobre el shadow map
// ---------------------------------------------------------------------------
float ComputeShadow(float4 shadowPosH)
{
    // Division perspectiva (la proyeccion ortografica tiene w=1 pero el calculo es correcto igual)
    float3 proj = shadowPosH.xyz / shadowPosH.w;

    // Convertir NDC [-1,1] a UV [0,1]; invertir Y por la convencion D3D
    float2 uv;
    uv.x =  proj.x * 0.5f + 0.5f;
    uv.y = -proj.y * 0.5f + 0.5f;

    // Fuera del shadow map -> completamente iluminado
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || proj.z > 1.0f)
        return 1.0f;

    float depth      = proj.z - ShadowBias;
    float texelSize  = 1.0f / 2048.0f;
    float shadow     = 0.0f;

    [unroll] for (int x = -1; x <= 1; ++x)
    {
        [unroll] for (int y = -1; y <= 1; ++y)
        {
            float smDepth = txShadow.Sample(samLinear, uv + float2(x, y) * texelSize).r;
            shadow += (depth <= smDepth) ? 1.0f : 0.0f;
        }
    }
    return shadow / 9.0f;
}

// ---------------------------------------------------------------------------
//  Pixel Shader: Lambert difuso + Blinn-Phong especular + ambiente + sombras PCF
// ---------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
    float3 N = normalize(input.WorldNormal);
    float3 L = normalize(-LightDir);                       // Hacia la luz
    float3 V = normalize(CameraPos - input.WorldPos);      // Hacia la camara
    float3 H = normalize(L + V);                           // Half-vector

    // Difuso Lambert
    float  NdotL   = saturate(dot(N, L));
    float3 ambient = 0.08f * LightColor;
    float3 diffuse = NdotL * LightColor;

    // Especular Blinn-Phong
    float  shininess = max(1.0f, (1.0f - Roughness) * 256.0f);
    float  NdotH     = saturate(dot(N, H));
    float3 specular  = pow(NdotH, shininess) * LightColor * (1.0f - Roughness);

    // Shadow factor PCF 3x3
    float shadowFactor = ComputeShadow(input.ShadowPosH);

    float4 albedo = txDiffuse.Sample(samLinear, input.Tex) * BaseColor;
    // Ambient siempre visible; diffuse y specular atenuados por la sombra
    float3 color  = ambient * albedo.rgb
                  + shadowFactor * (diffuse * albedo.rgb + specular);

    return float4(color, albedo.a);
}

// ---------------------------------------------------------------------------
//  Pixel Shader de sombra plana (proyeccion sobre el suelo)
// ---------------------------------------------------------------------------
float4 ShadowPS(PS_INPUT input) : SV_Target
{
    return float4(0.0f, 0.0f, 0.0f, 0.5f);
}
