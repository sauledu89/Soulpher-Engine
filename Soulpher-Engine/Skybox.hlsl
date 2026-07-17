// =============================================================================
//  Skybox.hlsl  |  Cubo unitario + panoramica equirectangular, detras de la escena.
//  El VS fuerza z=w (profundidad 1.0 tras la division perspectiva) para que el
//  depth test LESS_EQUAL del pipeline solo deje pasar los pixeles donde ningun
//  objeto opaco escribio un depth mas cercano.
//
//  La textura es una panoramica 2:1 (equirectangular), no un TextureCube: el PS
//  convierte la direccion 3D interpolada a coordenadas UV esfericas (atan2/asin)
//  en vez de usar TextureCube::Sample.
// =============================================================================

cbuffer CBSkybox : register(b0)
{
    matrix ViewProj;
}

Texture2D    SkyboxTex     : register(t7);
SamplerState SkyboxSampler : register(s0);

struct VS_INPUT
{
    float3 pos : POSITION;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 dir : TEXCOORD0;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.dir = input.pos;

    float4 clipPos = mul(float4(input.pos, 1.0f), ViewProj);
    output.pos = clipPos.xyww; // z = w -> profundidad siempre 1.0 (el fondo detras de todo)
    return output;
}

static const float kInvTwoPi = 0.15915494f; // 1 / (2*PI)
static const float kInvPi    = 0.31830989f; // 1 / PI

// Mapea una direccion 3D normalizada a UV de una panoramica equirectangular:
// fila superior de la imagen = cenit (arriba), fila inferior = nadir (abajo).
float2 DirectionToEquirectUV(float3 dir)
{
    float2 uv;
    uv.x = atan2(dir.z, dir.x) * kInvTwoPi + 0.5f;
    uv.y = 0.5f - asin(dir.y) * kInvPi;
    return uv;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
    float2 uv = DirectionToEquirectUV(normalize(input.dir));
    return SkyboxTex.Sample(SkyboxSampler, uv);
}
