#ifndef SAMPLER_GLSL
#define SAMPLER_GLSL

#include "Collisions.glsl"
#include "Shading.glsl"
#include "Random.glsl"

struct SampleInfo
{
	vec3 Direction;
	float Throughput;

	vec3 iNormal;
	float Weight;

	bool IsInvalid;
	bool IsReflected;
};

uint sRandomGeneratorSeed;
#define POWER_HEURISTICS_EXP      2.0
#define SAMPLE_SIZE               3

float GetDiffuseSpecularSamplingSeparation(float Roughness, float Metallic)
{
	return clamp((Roughness * Roughness + 1.0 - Metallic) / 2.0, 0.0, 1.0);
}

vec2 GetTransmissionProbability(float TransmissionWeight, float Metallic, 
	float RefractiveIndex, float NdotV, in vec3 RefractionDir)
{
	float Reflectivity = (RefractiveIndex - 1.0) / (RefractiveIndex + 1.0);
	Reflectivity *= Reflectivity;
	
	vec3 ReflectionFresnel = FresnelSchlick(NdotV, vec3(Reflectivity));
	float ReflectionProb = length(ReflectionFresnel);
	float TransmissionProb = clamp(1.0 - ReflectionProb, 0.0, 1.0) * TransmissionWeight * (1.0 - Metallic);

	//TransmissionProb = TransmissionWeight;
	//TransmissionProb = 0.0;

	TransmissionProb = length(RefractionDir) == 0.0 ? 0.0 : TransmissionProb;

	return vec2(1.0 - TransmissionProb, TransmissionProb);
}

vec3 GetSampleProbablities(float Roughness, float Metallic, float NdotV,
	float TransmissionWeight, float RefractiveIndex, in vec3 RefractionDir)
{
	// [ReflectionDiffuse, ReflectionSpecular, TransmissionDiffuse, TransmissionSpecular]

	float ScatteringSplit = GetDiffuseSpecularSamplingSeparation(Roughness, Metallic);
	vec2 TransmissionSplit = GetTransmissionProbability(TransmissionWeight, Metallic, 
		RefractiveIndex, NdotV, RefractionDir);

	vec3 Probs = vec3(0.0, 0.0, 0.0);

	Probs[0] = (1.0 - ScatteringSplit) * TransmissionSplit[0];
	Probs[1] = ScatteringSplit * TransmissionSplit[0];
	Probs[2] = TransmissionSplit[1];

	return Probs;
}

ivec2 GetSampleIndex(in vec3 SampleProbablities)
{
	float SampleCoin = GetRandom(sRandomGeneratorSeed);
	
	// Accumulate probabilities
    float p0 = SampleProbablities.x;
    float p1 = p0 + SampleProbablities.y;
    float p2 = p1 + SampleProbablities.z;

    // Use step to determine in which region the randomValue falls and cast to int
    int s1 = int(step(p0, SampleCoin)); // 1 if randomValue >= p0, else 0
    int s2 = int(step(p1, SampleCoin)); // 1 if randomValue >= p1, else 0
    int s3 = int(step(p2, SampleCoin)); // 1 if randomValue >= p2, else 0

    // Calculate the index by summing the step results and adding 0.5 offsets
    int SampleIndex = s1 + s2 + s3;
	int Hemisphere = SampleIndex == 2 ? -1 : 1;

	return ivec2(SampleIndex, Hemisphere);
}

// Samples Cook Torrance BSDF

void SampleUnitVec(inout SampleInfo sampleInfo, in CollisionInfo HitInfo, in vec3 IncomingDirection, float RefractiveIndex)
{
	// Getting necessary info and preparing...
	vec3 Normal = HitInfo.Normal;
	vec3 ViewDir = -IncomingDirection;
	float Roughness = HitInfo.ObjectMaterial.Roughness;
	float Metallic = HitInfo.ObjectMaterial.Metallic;
	float TransmissionWeight = HitInfo.ObjectMaterial.Transmission;

	float NdotV = dot(Normal, ViewDir);

	float Reflectivity = (RefractiveIndex - 1.0) / (RefractiveIndex + 1.0);
	Reflectivity = min(Reflectivity * Reflectivity, 1.0);

	vec3 Directions[SAMPLE_SIZE];
	vec3 HalfVecs[SAMPLE_SIZE];
	float SampleWeights[SAMPLE_SIZE];

	// Visible normal sampling
	HalfVecs[0] = SampleHalfVecGGXVNDF_Distribution(ViewDir, Normal, Roughness, sRandomGeneratorSeed);
	
	//if (Roughness < 0.1)
	//	HalfVecs[0] = Normal;

	// Cosine weighted sampling
	Directions[1] = SampleUnitVecCosineWeighted(Normal, sRandomGeneratorSeed);

	HalfVecs[2] = HalfVecs[0];
	HalfVecs[1] = normalize(ViewDir + Directions[1]);

	Directions[0] = reflect(-ViewDir, HalfVecs[0]);
	Directions[2] = refract(-ViewDir, HalfVecs[0], RefractiveIndex);

	// Getting sampling probablity for each aspect of BSDF
	vec3 SampleProbabilities = GetSampleProbablities(Roughness, Metallic, NdotV,
		TransmissionWeight, RefractiveIndex, Directions[2]);

	ivec2 SampleIndexInfo = GetSampleIndex(SampleProbabilities);
	int SampleIndex = SampleIndexInfo[0];
	float HemisphereSide = float(SampleIndexInfo[1]);

	vec3 LightDir = normalize(Directions[SampleIndex]);
	vec3 H = HalfVecs[SampleIndex];

	// Visible normal sample weights for both BRDF and BTDF
	float VNDF_SampleWeightReflection = PDF_GGXVNDF_Reflection(Normal, ViewDir, H, Roughness);
	float VNDF_SampleWeightRefraction = PDF_GGXVNDF_Refraction(-Normal, ViewDir, -H,
		LightDir, Roughness, RefractiveIndex);

	float CosineSampleWeight = max(abs(dot(Normal, LightDir)), SHADING_TOLERENCE) / MATH_PI;

	SampleWeights[0] = VNDF_SampleWeightReflection;
	SampleWeights[1] = CosineSampleWeight;
	SampleWeights[2] = VNDF_SampleWeightRefraction;

	// BRDF Sampling
	float SampleWeightInvReflection = (pow(SampleProbabilities[0] * SampleWeights[0], POWER_HEURISTICS_EXP) +
		pow(SampleProbabilities[1] * SampleWeights[1], POWER_HEURISTICS_EXP)) /
		pow(SampleProbabilities[SampleIndex] * SampleWeights[SampleIndex], POWER_HEURISTICS_EXP - 1.0);

	// BTDF Sampling
	float SampleWeightInvRefraction = (SampleProbabilities[2] * SampleWeights[2]);

	// Combining the BRDF and BTDF sampling
	float SampleWeightInv = HemisphereSide > 0.0 ? SampleWeightInvReflection : SampleWeightInvRefraction;

	// Russian roulette weight
	float NdotH = max(abs(dot(Normal, H)), SHADING_TOLERENCE);

	float FresnelFactor = min(FresnelSchlick(NdotH, vec3(Reflectivity)).r, 1.0);
	float GGX_Mask = min(GeometrySmith(NdotV, abs(dot(LightDir, Normal)), Roughness), 1.0);

	// Accumulating all the information
	sampleInfo.Direction = LightDir;
	sampleInfo.Weight = 1.0 / max(SampleWeightInv, SHADING_TOLERENCE);
	sampleInfo.iNormal = H;
	sampleInfo.Throughput = FresnelFactor * GGX_Mask;
	sampleInfo.IsInvalid = dot(LightDir, Normal) * HemisphereSide <= 0.0 || SampleIndex > 2;
	sampleInfo.IsReflected = HemisphereSide > 0.0;
}

#endif