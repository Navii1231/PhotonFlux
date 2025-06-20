#version 440 core

layout(location = 0) out vec4 FragColor;

struct Material
{
	vec4 BaseColor;
	float Roughness;
	float Metallic;
	float RefractiveIndex;
	float TransmissionWeight;
};

struct DirectionalLightSrc
{
    vec3 Direction;
};

struct PointLightSrc
{
    vec3 Position;
    vec3 Intensity;
    vec2 DropRate;
};

struct CookTorranceBSDF_Input
{
    vec3 ViewDir;
    float Roughness;

    vec3 Normal;
    float Metallic;

    vec3 BaseColor;
    vec3 LightDir;
    float RefractiveIndex;
};

vec3 FresnelSchlick(float HdotV, in vec3 Reflectivity)
{
    return vec3(Reflectivity) + vec3(1.0 - Reflectivity) * pow(1.0 - HdotV, 5.0);
}

float GeometrySchlickGGX(float DotProduct, float Roughness)
{
    float RoughnessSq = Roughness * Roughness;
    float DotProdClamped = max(DotProduct, SHADING_TOLERENCE);

    return (2.0 * DotProdClamped) / (DotProdClamped + sqrt(RoughnessSq +
        (1 - RoughnessSq) * DotProdClamped * DotProdClamped));
}

// Compute geometry term using Smith's method
float GeometrySmith(float NdotV, float NdotL, float Roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, Roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, Roughness);
    return ggx1 * ggx2;
}

// Compute normal distribution function using GGX/Trowbridge-Reitz
float DistributionGGX(float NdotH, float Roughness)
{
    NdotH = max(NdotH, SHADING_TOLERENCE);

    float a2 = Roughness * Roughness;
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = MATH_PI * denom * denom;

    float Value = num / denom;

    const float MaxGGX_Value = 1.0 / (2.0 * SHADING_TOLERENCE);

    return min(Value, MaxGGX_Value);
}

vec3 CookTorranceBRDF(in CookTorranceBSDF_Input bsdfInput)
{
#if _DEBUG
    if (dot(V, N) < 0.0)
        return vec3(1.0, 0.0, 0.0);

    if (dot(L, N) < 0.0)
        return vec3(1.0, 0.0, 0.0);
#endif

	vec3 ViewDir = bsdfInput.ViewDir;
    vec3 LightDir = bsdfInput.LightDir;

    vec3 iNormal = normalize(bsdfInput.ViewDir + bsdfInput.LightDir);

    float NdotH = max(dot(bsdfInput.Normal, iNormal), SHADING_TOLERENCE);
    float NdotV = max(dot(bsdfInput.Normal, ViewDir), SHADING_TOLERENCE);
    float NdotL = max(dot(bsdfInput.Normal, LightDir), SHADING_TOLERENCE);
	float HdotV = max(dot(iNormal, ViewDir), SHADING_TOLERENCE);

    vec3 Reflectivity = vec3((bsdfInput.RefractiveIndex - 1.0) /
        (bsdfInput.RefractiveIndex + 1.0));

    Reflectivity *= Reflectivity;

    Reflectivity = mix(Reflectivity, bsdfInput.BaseColor, bsdfInput.Metallic);

    float NDF = DistributionGGX(NdotH, bsdfInput.Roughness);
    float G = GeometrySmith(NdotL, NdotV, bsdfInput.Roughness);
    vec3 F = FresnelSchlick(NdotH, Reflectivity);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL;

    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS);
    kD *= 1.0 - bsdfInput.Metallic;

    vec3 Lo = (kD * bsdfInput.BaseColor / MATH_PI + specular) * NdotL;
    //Lo = specular * NdotL;
    //Lo = (kD * bsdfInput.BaseColor / MATH_PI) * NdotL;

	return Lo;
}

layout(set = 0, binding = 0) uniform sampler2D uPositions;
layout(set = 0, binding = 1) uniform sampler2D uNormals;
layout(set = 0, binding = 2) uniform sampler2D uTangents;
layout(set = 0, binding = 3) uniform sampler2D uBitangents;
layout(set = 0, binding = 4) uniform sampler2D uTexCoords;
layout(set = 0, binding = 5) uniform sampler2D uMaterialIDs;

layout(std430, set = 1, binding = 0) buffer MaterialBuffer
{
	Material sMaterials[];
};

layout(std430, set = 1, binding = 1) buffer DirectionalLights
{
    DirectionalLightSrc sDirectionalLightSrc[];
};

layout(std140, set = 1, binding = 2) uniform Camera
{
    mat4 uProjection;
	mat4 uView;
};

layout(location = 0) in vec2 vTexCoords;

void main()
{
	vec3 Position = texture(uPositions, vTexCoords).xyz;
    vec3 Normal = normalize(texture(uNormals, vTexCoords).xyz);
    vec3 Tangent = normalize(texture(uTangents, vTexCoords).xyz);
    vec3 Bitangent = normalize(texture(uBitangents, vTexCoords).xyz);
    vec2 TexCoords = texture(uTexCoords, vTexCoords).xy;
    uvec2 MaterialID = uvec2(texture(uMaterialIDs, vTexCoords).xy + vec2(0.5));

    Material pbrMat = sMaterials[MaterialID.x];

    CookTorranceBSDF_Input bsdfIn;

    bsdfIn.ViewDir = normalize(Position - uView[3].xyz);
    bsdfIn.LightDir = -normalize(sDirectionalLightSrc[0].Direction);
    bsdfIn.BaseColor = pbrMat.BaseColor.rgb;
    bsdfIn.Roughness = pbrMat.Roughness;
    bsdfIn.Metallic = pbrMat.Metallic;
    bsdfIn.Normal = Normal;
    bsdfIn.RefractiveIndex = pbrMat.RefractiveIndex;

    vec3 Luminance = CookTorranceBRDF(bsdfIn);

    FragColor = vec4(Luminance, pbrMat.BaseColor.a);
}
