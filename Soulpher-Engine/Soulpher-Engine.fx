//--------------------------------------------------------------------------------------
// Recursos: textura y sampler
//--------------------------------------------------------------------------------------

Texture2D txDiffuse : register(t0); // Textura base aplicada a la malla.
SamplerState samLinear : register(s0); // Sampler con filtrado lineal.

//--------------------------------------------------------------------------------------
// Buffers constantes
//--------------------------------------------------------------------------------------

/**
 * @brief Matriz de vista (cámara). No cambia durante la ejecución.
 */
cbuffer cbNeverChanges : register(b0)
{
    matrix View;
}

/**
 * @brief Matriz de proyección. Cambia si la ventana cambia de tamaño.
 */
cbuffer cbChangeOnResize : register(b1)
{
    matrix Projection;
}

/**
 * @brief Datos que cambian por cada frame: mundo y color del mesh.
 */
cbuffer cbChangesEveryFrame : register(b2)
{
    matrix World; // Matriz de transformación del objeto.
    float4 vMeshColor; // Color base del objeto (puede incluir opacidad).
}

//--------------------------------------------------------------------------------------
// Estructuras para la comunicación entre etapas del pipeline
//--------------------------------------------------------------------------------------

/**
 * @struct VS_INPUT
 * @brief Entrada del vertex shader.
 */
struct VS_INPUT
{
    float4 Pos : POSITION; // Posición del vértice en espacio local.
    float2 Tex : TEXCOORD0; // Coordenadas de textura.
};

/**
 * @struct PS_INPUT
 * @brief Salida del vertex shader / entrada del pixel shader.
 */
struct PS_INPUT
{
    float4 Pos : SV_POSITION; // Posición en espacio de recorte.
    float2 Tex : TEXCOORD0; // Coordenadas de textura interpoladas.
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

/**
 * @brief Shader de vértices: transforma la posición del vértice a espacio de recorte.
 *
 * Aplica las transformaciones World, View y Projection en orden.
 *
 * @param input Entrada del vértice con posición y textura.
 * @return PS_INPUT Estructura con datos transformados.
 */
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // Transformación a espacio mundo
    output.Pos = mul(input.Pos, World);
    // Transformación a espacio vista
    output.Pos = mul(output.Pos, View);
    // Transformación a espacio de proyección
    output.Pos = mul(output.Pos, Projection);

    // Pasa la coordenada de textura sin modificar
    output.Tex = input.Tex;

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader: renderizado con textura y color base
//--------------------------------------------------------------------------------------

/**
 * @brief Pixel shader para renderizado estándar.
 *
 * Aplica la textura difusa multiplicada por el color del mesh.
 * Se usa para render normal del objeto en pantalla.
 *
 * @param input Datos interpolados desde el vertex shader.
 * @return float4 Color final del píxel.
 */
float4 PS(PS_INPUT input) : SV_Target
{
    return txDiffuse.Sample(samLinear, input.Tex) * vMeshColor;
}

//--------------------------------------------------------------------------------------
// Pixel Shader: sombra con transparencia
//--------------------------------------------------------------------------------------

/**
 * @brief Pixel shader para dibujar sombras simples.
 *
 * Devuelve un color negro semitransparente (alpha = 0.5).
 * Ideal para proyectar una sombra en la escena.
 *
 * @param input Datos interpolados desde el vertex shader.
 * @return float4 Color de sombra.
 */
float4 ShadowPS(PS_INPUT input) : SV_Target
{
    return float4(0.0f, 0.0f, 0.0f, 0.5f);
}

//--------------------------------------------------------------------------------------
// Técnica para usar los shaders
//--------------------------------------------------------------------------------------

/**
 * @technique Render
 * @brief Técnica principal para renderizado.
 *
 * Compila y vincula los shaders definidos anteriormente.
 */
technique10 Render
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetPixelShader(CompileShader(ps_4_0, PS()));
    }
}

//--------------------------------------------------------------------------------------
// Instrucciones para compilar shaders manualmente con fxc:
// fxc /T vs_4_0 /E VS         ShadowTester.fx
// fxc /T ps_4_0 /E PS         ShadowTester.fx
// fxc /T ps_4_0 /E ShadowPS   ShadowTester.fx
//--------------------------------------------------------------------------------------
