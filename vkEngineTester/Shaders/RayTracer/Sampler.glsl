#ifndef SAMPLER_GLSL
#define SAMPLER_GLSL

#include "Collisions.glsl"
#include "Shading.glsl"
#include "Random.glsl"

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

	TransmissionProb = length(RefractionDir) == 0.0 ? 0.0 : TransmissionProb;

	//TransmissionProb = TransmissionWeight;

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

bool SampleUnitVec(inout StackEntry Entry, float RefractiveIndex)
{	
	vec3 Normal = Entry.HitInfo.Normal;
	vec3 ViewDir = -Entry.OutgoingRay.Direction;
	float Roughness = Entry.HitInfo.ObjectMaterial.Roughness;
	float Metallic = Entry.HitInfo.ObjectMaterial.Metallic;
	float TransmissionWeight = Entry.HitInfo.ObjectMaterial.Transmission;

	float NdotV = dot(Normal, ViewDir);

	vec3 Directions[SAMPLE_SIZE];
	vec3 HalfVecs[SAMPLE_SIZE];
	float SampleWeights[SAMPLE_SIZE];

	HalfVecs[0] = SampleHalfVecGGXVNDF_Distribution(ViewDir, Normal, Roughness, sRandomGeneratorSeed);
	Directions[1] = SampleUnitVecCosineWeighted(Normal, sRandomGeneratorSeed);

	HalfVecs[2] = HalfVecs[0];

	HalfVecs[1] = normalize(ViewDir + Directions[1]);

	Directions[0] = reflect(-ViewDir, HalfVecs[0]);
	Directions[2] = refract(-ViewDir, HalfVecs[0], RefractiveIndex);

	vec3 SampleProbabilities = GetSampleProbablities(Roughness, Metallic, NdotV,
		TransmissionWeight, RefractiveIndex, Directions[0]);

	ivec2 SampleIndexInfo = GetSampleIndex(SampleProbabilities);
	int SampleIndex = SampleIndexInfo[0];
	float HemisphereSide = float(SampleIndexInfo[1]);

	vec3 LightDir = normalize(Directions[SampleIndex]);
	vec3 H = HalfVecs[SampleIndex];

	float VNDF_SampleWeightReflection = PDF_GGXVNDF_Reflection(Normal, ViewDir, H, Roughness);
	float VNDF_SampleWeightRefraction = PDF_GGXVNDF_Refraction(-Normal, ViewDir, -H,
		LightDir, Roughness, RefractiveIndex);

	float CosineSampleWeight = max(abs(dot(Normal, LightDir)), SHADING_TOLERENCE) / MATH_PI;

	SampleWeights[0] = VNDF_SampleWeightReflection;
	SampleWeights[1] = CosineSampleWeight;
	SampleWeights[2] = VNDF_SampleWeightRefraction;

	float SampleWeightInvReflection = (pow(SampleProbabilities[0] * SampleWeights[0], POWER_HEURISTICS_EXP) +
		pow(SampleProbabilities[1] * SampleWeights[1], POWER_HEURISTICS_EXP)) / 
		pow(SampleProbabilities[SampleIndex] * SampleWeights[SampleIndex], POWER_HEURISTICS_EXP - 1.0);

	//float SampleWeightInvReflection = (SampleProbabilities[0] * SampleWeights[0] +
	//	SampleProbabilities[1] * SampleWeights[1]);

	float SampleWeightInvRefraction = (SampleProbabilities[2] * SampleWeights[2]);

	float SampleWeightInv = HemisphereSide > 0.0 ? SampleWeightInvReflection : SampleWeightInvRefraction;

	Entry.HalfVec = H;
	Entry.IncomingRay.Origin = Entry.HitInfo.IntersectionPoint +
		HemisphereSide * TOLERENCE * Entry.HitInfo.Normal;
	Entry.IncomingRay.Direction = LightDir;
	Entry.IncomingSampleWeight = SampleWeightInv;
	Entry.InterfaceRefractiveIndex = 1.0 / RefractiveIndex;

	return dot(LightDir, Normal) * HemisphereSide > 0.0;
}


#endif