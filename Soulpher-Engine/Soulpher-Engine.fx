//--------------------------------------------------------------------------------------
// Recursos: textura y sampler
//--------------------------------------------------------------------------------------

Texture2D txDiffuse : register(t0); // Textura base aplicada a la malla.
SamplerState samLinear : register(s0); // Sampler con filtrado lineal.

//--------------------------------------------------------------------------------------
// Buffers constantes
//--------------------------------------------------------------------------------------

/**
 * @brief Matriz de vista (c�mara). No cambia durante la ejecuci�n.
 */
cbuffer cbNeverChanges : register(b0)
{
    matrix View;
}

/**
 * @brief Matriz de proyecci�n. Cambia si la ventana cambia de tama�o.
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
    matrix World; // Matriz de transformaci�n del objeto.
    float4 vMeshColor; // Color base del objeto (puede incluir opacidad).
}

//--------------------------------------------------------------------------------------
// Estructuras para la comunicaci�n entre etapas del pipeline
//--------------------------------------------------------------------------------------

/**
 * @struct VS_INPUT
 * @brief Entrada del vertex shader.
 */
struct VS_INPUT
{
    float4 Pos : POSITION; // Posici�n del v�rtice en espacio local.
    float2 Tex : TEXCOORD0; // Coordenadas de textura.
};

/**
 * @struct PS_INPUT
 * @brief Salida del vertex shader / entrada del pixel shader.
 */
struct PS_INPUT
{
    float4 Pos : SV_POSITION; // Posici�n en espacio de recorte.
    float2 Tex : TEXCOORD0; // Coordenadas de textura interpoladas.
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

/**
 * @brief Shader de v�rtices: transforma la posici�n del v�rtice a espacio de recorte.
 *
 * Aplica las transformaciones World, View y Projection en orden.
 *
 * @param input Entrada del v�rtice con posici�n y textura.
 * @return PS_INPUT Estructura con datos transformados.
 */
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // Transformaci�n a espacio mundo
    output.Pos = mul(input.Pos, World);
    // Transformaci�n a espacio vista
    output.Pos = mul(output.Pos, View);
    // Transformaci�n a espacio de proyecci�n
    output.Pos = mul(output.Pos, Projection);

    // Pasa la coordenada de textura sin modificar
    output.Tex = input.Tex;

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader: renderizado con textura y color base
//--------------------------------------------------------------------------------------

/**
 * @brief Pixel shader para renderizado est�ndar.
 *
 * Aplica la textura difusa multiplicada por el color del mesh.
 * Se usa para render normal del objeto en pantalla.
 *
 * @param input Datos interpolados desde el vertex shader.
 * @return float4 Color final del p�xel.
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
// T�cnica para usar los shaders
//--------------------------------------------------------------------------------------

/**
 * @technique Render
 * @brief T�cnica principal para renderizado.
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
