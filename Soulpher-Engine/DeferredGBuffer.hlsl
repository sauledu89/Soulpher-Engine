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

// [GameDev] Los 6 floats de padding al final no son basura: HLSL empaqueta cbuffers en
// bloques de 16 bytes (registros float4), así que 7 floats sueltos (BaseColor ya es un
// float4 aparte) quedarían mal alineados con el struct CBPerMaterial de C++ si no se
// completan hasta el siguiente múltiplo de 16 bytes. Si agregas o quitas un campo aquí,
// DEBES actualizar CBPerMaterial en Prerequisites.h en el mismo commit — un desalineamiento
// silencioso hace que valores como AlphaCutoff lean el byte equivocado sin ningún error de
// compilación, solo resultados visualmente incorrectos.
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
// [GameDev] Este PS NO calcula iluminación — solo escribe datos "crudos" de superficie a
// los 4 MRTs. Toda la matemática de Lambert/Blinn-Phong vive en DeferredLighting.hlsl,
// que corre UNA vez por píxel final en vez de una vez por triángulo dibujado aquí. Es la
// separación fundamental del Deferred Rendering: geometry pass = "qué hay en cada píxel",
// lighting pass = "cómo se ve iluminado".
PS_OUTPUT PS(VS_OUTPUT input)
{
    // Sample albedo; apply material base color tint
    float4 albedo = AlbedoTexture.Sample(Sampler, input.tex) * BaseColor;

    // [GameDev] clip() descarta el píxel (no escribe a NINGÚN render target, incluida la
    // profundidad) si el argumento es negativo. Para materiales Masked (follaje, rejillas,
    // alambre de púas) esto crea "agujeros" reales en la geometría a nivel de píxel sin
    // necesitar geometría separada ni alpha blending — a diferencia de blending, un píxel
    // clip()eado SÍ actualiza el depth buffer normalmente para los píxeles que sobreviven,
    // así que sigue proyectando sombra y ocluyendo correctamente. AlphaCutoff es 0 para
    // materiales no-Masked (ver DeferredRenderer::renderGeometryObject), así que clip()
    // nunca descarta nada en ese caso (alpha - 0 >= 0 siempre, salvo alpha negativo).
    clip(albedo.a - AlphaCutoff);

    // Geometric surface basis (TBN), re-ortogonalizada por si la interpolación las desalineó.
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent - N * dot(input.tangent, N));
    float3 B = normalize(input.bitangent);

    // [GameDev] Normal mapping en tangent-space: la textura guarda perturbaciones RELATIVAS
    // a la superficie (no direcciones absolutas en world-space), codificadas como color RGB
    // en [0,1] donde (0.5,0.5,1.0) = "sin relieve". Se decodifica a [-1,1], se atenúa con
    // NormalScale (útil para debug o para materiales con normal map muy agresivo) y se
    // transforma de tangent-space a world-space multiplicando por la matriz TBN (Tangent-
    // Bitangent-Normal) — el mismo truco que usa prácticamente cualquier motor con normal
    // mapping, desde Doom 3 en adelante.
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

    // [GameDev] RT1 es UNORM (8 bits/canal para el ejemplo típico, o el float del formato
    // elegido) y no puede almacenar negativos, pero una normal world-space válida tiene
    // componentes en [-1,1]. Este es el mismo "encode" que se usa en prácticamente todo
    // G-Buffer: mapear [-1,1] -> [0,1] al escribir (aquí) y revertirlo con *2-1 al leer
    // (ver DeferredLighting.hlsl). Sin este paso, cualquier normal con componente negativa
    // se "clampearía" a 0 al escribirse, destruyendo la iluminación de media escena.
    float3 encodedNormal = N * 0.5f + 0.5f;

    // No emissive texture — use strength from CB
    float3 emissive = albedo.rgb * EmissiveStrength;

    // [GameDev] El layout de cada RT (qué va en RGB, qué en A) es un contrato estricto con
    // DeferredLighting.hlsl: cambiar el orden aquí sin actualizar el unpack del lighting
    // pass produce errores silenciosos — ej. leer roughness donde debería ir metallic — que
    // no generan ningún warning de compilación, solo un render visualmente incorrecto.
    PS_OUTPUT output;
    output.RT0 = float4(albedo.rgb,    metallic);       // Albedo + Metallic
    output.RT1 = float4(encodedNormal, roughness);      // Normal + Roughness
    output.RT2 = float4(input.worldPos, ao);            // WorldPos + AO
    output.RT3 = float4(emissive, albedo.a);            // Emissive + Alpha

    return output;
}
