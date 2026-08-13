/**
 * @file Soulpher-Engine.fx
 * @brief Shader principal de Soulpher-Engine — Forward Rendering con shadow maps y Blinn-Phong.
 *
 * @details
 * Este archivo HLSL implementa el pipeline de render completo del motor:
 *
 *  **Constant Buffers** (deben coincidir byte-a-byte con RenderTypes.h):
 *   - b0 CBPerFrame:    View, Projection, LightViewProjection, CameraPos, LightDir, LightColor, LightRange, multi-luz.
 *   - b1 CBPerObject:   World matrix (una por draw call).
 *   - b2 CBPerMaterial: BaseColor, Metallic, Roughness, AO, NormalScale, EmissiveStrength, AlphaCutoff + pad.
 *
 *  **Recursos**:
 *   - t0 txDiffuse: textura de albedo/color base.
 *   - t6 txShadow:  shadow map R24_UNORM_X8_TYPELESS (escrito por el shadow pass).
 *   - s0 samLinear: sampler bilineal con wrap.
 *
 *  **Modelo de iluminacion** (PS principal):
 *   - Difuso Lambert: NdotL * LightColor
 *   - Especular Blinn-Phong: pow(NdotH, shininess) — usa half-vector H en vez del vector R de Phong
 *   - Ambiente flat: 0.08 * LightColor (siempre visible, no atenuada por sombras)
 *   - Shadow factor PCF 3x3: suaviza los bordes de la sombra con 9 muestras
 *
 *  **Shadow map** (ComputeShadow):
 *   - Convierte ShadowPosH de espacio de clip a UV [0,1] con la transformacion NDC->UV.
 *   - Aplica bias constante (0.003f) para evitar shadow acne (auto-sombreado por error de precision).
 *   - PCF promedia 9 muestras en una grilla 3x3 para suavizar los bordes.
 *
 * @note [GameDev] Blinn-Phong usa el half-vector H = normalize(L+V) en vez del vector de
 * reflexion R de Phong clasico. Es mas rapido (no necesita reflect()) y da resultados
 * fisicamente mas correctos para superficies rugosas. Es el modelo de especular de
 * OpenGL 2.x/3.x y de DX9. PBR (Cook-Torrance BRDF) extiende esto con GGX NDF,
 * Fresnel (Schlick), y Geometria (Smith), que es lo que Unreal Engine y Unity usan.
 * Para escalar este shader a PBR real, el paso siguiente seria reemplazar el bloque
 * de especular con la BRDF Cook-Torrance, usando Metallic para separar dielectrico/metal.
 *
 * @note [GameDev] La formula NDC-to-UV es:
 *   uv.x =  proj.x * 0.5 + 0.5  (NDC [-1,1] -> UV [0,1])
 *   uv.y = -proj.y * 0.5 + 0.5  (Y se invierte: NDC +1 = arriba, UV 0 = arriba en D3D)
 * OpenGL no invierte Y porque su UV origin esta abajo-izquierda. D3D tiene UV origin arriba.
 *
 * @see ForwardRenderer.h, RenderTypes.h, ForwardRenderer.cpp, CBPerFrame, MaterialInstance
 */
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

// b0 — datos por frame: camara + luz principal + arreglos multi-luz
// IMPORTANTE: debe coincidir byte-a-byte con CBPerFrame en RenderTypes.h
cbuffer CBPerFrame : register(b0)
{
    matrix View;
    matrix Projection;
    matrix LightViewProjection;   // View*Proj de la luz (shadow map)
    float3 CameraPos;
    float  _pad0;
    float3 LightDir;              // Direccion DE la luz principal (normalizada)
    float  _pad1;
    float3 LightColor;            // Color de la luz principal
    float  LightRange;            // Radio de la luz principal (Point/Spot)
    float3 LightPosition;         // Posicion de la luz principal
    int    LightType;             // 0=Directional, 1=Point, 2=Spot
    float4 LightPositionsRanges[8];          // xyz=posicion, w=range de cada luz
    float4 LightColorsTypes[8];              // xyz=color, w=tipo de cada luz
    float4 LightDirectionsIntensities[8];    // xyz=direccion, w=intensidad de cada luz
    int    LightCount;            // Numero de luces activas en este frame
    float3 _pad2;
}

// b1 — datos por objeto: transformacion de mundo
cbuffer CBPerObject : register(b1)
{
    matrix World;
}

// b2 — datos por material: color base y parametros PBR
// Nota: pad2-pad5 reservados para futuras extensiones (total = 64 bytes)
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
    float2 UVTiling;    // Repeticiones de textura en U/V.
    float2 UVOffset;    // Desplazamiento de UV tras el tiling — mismo layout binario que los _mpad2-_mpad5 que reemplaza.
}

// ---------------------------------------------------------------------------
//  Estructuras de pipeline
// ---------------------------------------------------------------------------

/**
 * @struct VS_INPUT
 * @brief Datos de entrada del Vertex Shader, one-to-one con SimpleVertex en C++.
 *
 * El Input Layout (definido en ShaderProgram) mapea los campos de SimpleVertex
 * a estos semantics. El stride es sizeof(SimpleVertex) = 56 bytes.
 */
struct VS_INPUT
{
    float3 Pos       : POSITION;   ///< Posicion en espacio local del objeto.
    float3 Normal    : NORMAL;     ///< Normal de superficie (sin normalizar en VRAM).
    float3 Tangent   : TANGENT;    ///< Tangente para normal mapping (generada por ModelLoader).
    float3 Bitangent : BITANGENT;  ///< Bitangente = cross(Normal, Tangent) (base TBN completa).
    float2 Tex       : TEXCOORD0;  ///< Coordenadas UV para muestrear texturas.
};

/**
 * @struct PS_INPUT
 * @brief Datos interpolados que recibe el Pixel Shader desde el rasterizador.
 *
 * El rasterizador interpola linealmente estos valores entre los 3 vertices del triangulo.
 * ShadowPosH NO se divide por w aqui — ComputeShadow lo hace internamente.
 */
struct PS_INPUT
{
    float4 ClipPos    : SV_POSITION; ///< Posicion en clip space (requerida por SV_POSITION).
    float2 Tex        : TEXCOORD0;   ///< UVs interpoladas para muestreo de texturas.
    float3 WorldPos   : TEXCOORD1;   ///< Posicion en espacio mundo para calculo especular (V = CameraPos - WorldPos).
    float3 WorldNormal: TEXCOORD2;   ///< Normal transformada al espacio mundo (normalizar en PS).
    float4 ShadowPosH : TEXCOORD3;   ///< Posicion en espacio de clip de la luz (para shadow map lookup).
};

// ---------------------------------------------------------------------------
//  Vertex Shader principal
// ---------------------------------------------------------------------------

/**
 * @brief Vertex Shader: transforma vertices de espacio local a clip space.
 *
 * Calcula:
 *  - ClipPos:     World -> View -> Projection (MVP completo).
 *  - WorldPos:    posicion en espacio mundo para calculo de V y specular.
 *  - WorldNormal: normal en espacio mundo (mul con float3x3 de World).
 *  - ShadowPosH:  posicion en clip space de la luz (para ComputeShadow).
 *
 * @note Para escala NO uniforme, WorldNormal deberia usar InvTransposeWorld.
 * Con escala uniforme, mul(normal, (float3x3)World) es correcto.
 */
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

/**
 * @brief Calcula el factor de sombra [0,1] usando PCF 3x3.
 *
 * @param shadowPosH Posicion en clip space de la luz (ShadowPosH del PS_INPUT).
 * @return 0.0 = completamente en sombra, 1.0 = completamente iluminado.
 *
 * @details
 * Pasos:
 *  1. Division perspectiva: proj = shadowPosH.xyz / shadowPosH.w  (w=1 para ortho, pero correcto igual).
 *  2. NDC -> UV: uv.x = proj.x*0.5+0.5;  uv.y = -proj.y*0.5+0.5  (flip Y por convencion D3D).
 *  3. Fuera del frustum de la luz -> sin sombra (return 1.0).
 *  4. depth = proj.z - 0.003f  (bias constante previene shadow acne por error de punto flotante).
 *  5. PCF 3x3: 9 muestras en grilla alrededor del UV central, cada una comparada con depth.
 *  6. Factor final = suma / 9.
 *
 * @note [GameDev] PCF (Percentage Closer Filtering) NO filtra profundidades — compara
 * primero y promedia los resultados binarios (0/1). Esto suaviza el borde de la sombra.
 * Alternativas mas costosas: PCSS (Percentage Closer Soft Shadows, busca el bloqueador
 * mas cercano y varía el radio del PCF), o VSM (Variance Shadow Maps, mas eficiente
 * en bandas pero con light-bleeding artifacts). Todos los motores AAA usan alguna de estas.
 */
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

    float depth      = proj.z - 0.003f; // Shadow bias hardcodeado (previene shadow acne)
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

/**
 * @brief Pixel Shader principal: calcula el color final de cada fragmento.
 *
 * @param input Datos interpolados del rasterizador.
 * @return Color RGBA del fragmento (SV_Target = output al render target activo).
 *
 * @details Modelo de iluminacion:
 *  - N = WorldNormal normalizada.
 *  - L = normalize(-LightDir)  (de la superficie hacia la luz).
 *  - V = normalize(CameraPos - WorldPos)  (de la superficie hacia la camara).
 *  - H = normalize(L + V)  (half-vector para Blinn-Phong).
 *
 *  color = ambient + shadowFactor * (diffuse + specular)
 *
 *  - ambient  = 0.08 * LightColor * albedo.rgb  (iluminacion de relleno constante)
 *  - diffuse  = NdotL * LightColor * albedo.rgb  (Lambert)
 *  - specular = pow(NdotH, shininess) * LightColor * (1 - Roughness)  (Blinn-Phong)
 *  - shininess = (1 - Roughness) * 256  (mapeo de Roughness PBR a exponente Phong)
 *
 * @note El ambient NO se multiplica por shadowFactor: un pixel en sombra sigue
 * recibiendo luz ambiente. Esto evita areas completamente negras (sombras "sin rebotar").
 * En PBR real, el ambient se reemplaza con IBL (Image-Based Lighting) usando environment maps.
 */
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

    float4 albedo = txDiffuse.Sample(samLinear, input.Tex * UVTiling + UVOffset) * BaseColor;
    // Ambient siempre visible; diffuse y specular atenuados por la sombra
    float3 color  = ambient * albedo.rgb
                  + shadowFactor * (diffuse * albedo.rgb + specular);

    return float4(color, albedo.a);
}

// ---------------------------------------------------------------------------
//  Pixel Shader de sombra plana (proyeccion sobre el suelo) — legacy
// ---------------------------------------------------------------------------

/**
 * @brief Pixel Shader de sombra plana proyectada en el suelo.
 *
 * Devuelve negro semitransparente (alpha=0.5) para simular la sombra del actor
 * sobre un plano horizontal. La geometria se proyecta via una shadow projection
 * matrix calculada en Actor::renderShadow() antes de llamar a este shader.
 *
 * @note [GameDev] Esta es la tecnica de "fake shadow" o "blob shadow": barata pero
 * solo funciona en superficies planas. No funciona en terrenos irregulares ni
 * con objetos apilados. Es la tecnica de sombras de juegos PS1/N64 como Mario 64.
 * El shadow map implementado en ForwardRenderer la reemplaza para la mayoria de casos,
 * pero esta permanece como fallback o para efectos de sombra suave sobre el suelo.
 */
float4 ShadowPS(PS_INPUT input) : SV_Target
{
    return float4(0.0f, 0.0f, 0.0f, 0.5f);
}
