// -------------------------
//   Input/Ouput Structs
// -------------------------
struct VS_INPUT
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float4 WorldPos : WORLD;
    float2 UV : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float3 ViewDir : VIEWDIRECTION;
};

// -------------------------
//   WorldViewProjection Matrix
// -------------------------
float4x4 gWorldViewProj : WorldViewProjection;

// -------------------------
//  Textures
// -------------------------
Texture2D gDiffuseMap : DiffuseMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;
Texture2D gGlossinessMap : GlossinessMap;

// -------------------------
//   Sampler States (How to Sample Texture)
// -------------------------
SamplerState gSampler;

// -------------------------
//   Shading Variables
// -------------------------
float4x4 gWorldMatrix : WORLD;
float3 gCameraPos : CAMERA;

struct LIGHT
{
    float3 LightDirection : LightDir;
    float LightIntensity : Intensity;
};
static const LIGHT gLight1 = { normalize(float3(0.577f, -0.577f, 0.577f)), 1.f };

const float PI = 3.14159f;

// -------------------------
//   Shader Functions
// -------------------------

// Vertex Shader
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    output.Position = mul(float4(input.Position, 1.f), gWorldViewProj);
    output.WorldPos = mul(float4(input.Position, 1.f), gWorldMatrix);
    output.UV = input.UV;
    output.Normal = mul(normalize(input.Normal), (float3x3) gWorldMatrix).xyz; // Transformed Normal to World
    output.Tangent = mul(normalize(input.Tangent), (float3x3) gWorldMatrix).xyz; // Transformed Tangent to World
    output.ViewDir = gCameraPos - output.WorldPos.xyz;
    
    return output;
}

// Pixel Shader Logic
float3 GetLambertColor(in VS_OUTPUT input, float3 N, float3 lightDir)
{
    const float cosTheta = saturate(dot(N, lightDir));
    const float3 albedo = gDiffuseMap.Sample(gSampler, input.UV).rgb;
    const float diffuseReflectance = 7.f;
    
    const float3 LambertColor = (albedo * diffuseReflectance / PI) * cosTheta * gLight1.LightIntensity;
    
    return LambertColor;
}

float3 GetPhongColor(in VS_OUTPUT input, float3 N, float3 lightVector)
{
    const float lightHitNormal = dot(N, lightVector);
    
    if (lightHitNormal <= 0.f)
    {
        return float3(0, 0, 0);
    }
    
    const float3 sampledSpecular = gSpecularMap.Sample(gSampler, input.UV).rgb;
    const float sampledGlossR = gGlossinessMap.Sample(gSampler, input.UV).r;
    
    const float shininess = 25.f / gLight1.LightIntensity;
    const float phongExponent = sampledGlossR * shininess;
    
    //const float3 viewDirection = normalize(gCameraPos - input.WorldPos.xyz);
    const float3 viewDirection = normalize(input.ViewDir);
    
    const float3 reflectRay = reflect(-lightVector, N);
    const float cosA = saturate(dot(reflectRay, viewDirection));
    const float expCosA = pow(cosA, phongExponent);
    
    const float reflectance = sampledSpecular.r;
    
    const float3 SpecularColor = sampledSpecular * reflectance * expCosA * gLight1.LightIntensity;
    
    return SpecularColor;
}

float3 SampleNormal(float2 inputUV)
{
    float3 normal = gNormalMap.Sample(gSampler, inputUV).xyz;
    return normalize(normal * 2.f - 1.f);
}

float4 PS(VS_OUTPUT input) : SV_Target
{
    float3 N = normalize(input.Normal);
    const float3 lightDir = normalize(-gLight1.LightDirection);
    
    // Calculating World Normal
    const float3 T = normalize(input.Tangent);
    const float3 B = normalize(cross(N, T));
    
    const float3x3 TBN = float3x3(T, B, N);
    
    const float3 tangentSpaceNormal = SampleNormal(input.UV).rgb;
    const float3 worldNormal = normalize(mul(tangentSpaceNormal, TBN));
    
    const float3 LambertColor = GetLambertColor(input, worldNormal, lightDir);
    const float3 PhongColor = GetPhongColor(input, worldNormal, lightDir);
    const float3 AmbientColor = float3(0.025f, 0.025f, 0.025f);

    return float4(LambertColor + PhongColor + AmbientColor, 1.f);
}

// -------------------------
//   Technique
// -------------------------
RasterizerState gRasterizerStateBack
{
    CullMode = Back;
    FrontCounterClockwise = false;
};

RasterizerState gRasterizerStateNone
{
    CullMode = None;
    FrontCounterClockwise = false;
};

RasterizerState gRasterizerStateFront
{
    CullMode = Front;
    FrontCounterClockwise = false;
};

DepthStencilState gDepthStencilState
{
    DepthEnable = true;
    DepthWriteMask = all; // Set to ALL so it WRITES Depth, unlike the transparent shader
    DepthFunc = less;
    StencilEnable = false;
};

BlendState gBlendState
{
    BlendEnable[0] = false; // NO Blending
    RenderTargetWriteMask[0] = 0x0F;
};

technique11 DefaultTechnique
{
    pass P0
    {
        SetRasterizerState(gRasterizerStateNone);
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.f, 0.f, 0.f, 0.f), 0xFFFFFFFF);
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

    pass P1
    {
        SetRasterizerState(gRasterizerStateBack);
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.f, 0.f, 0.f, 0.f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

    pass P2
    {
        SetRasterizerState(gRasterizerStateFront);
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.f, 0.f, 0.f, 0.f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}