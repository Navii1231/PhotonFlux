#pragma once
#include "../PhotonFlux/RaytracingStructures.h"

#include <random>

class SampleGenerator
{
public:
	SampleGenerator();

	glm::vec3 GenerateUniformDistribution();
	glm::vec3 GenerateCosineWeightedDistribution(const glm::vec3 Normal);
	glm::vec3 GenerateTrowbritzGGX_Distribution(const glm::vec3 View, float Roughness);

	glm::vec3 GenerateGGXVNDF_Distribution(const glm::vec3& View, const glm::vec3& Normal, float Roughness);

	glm::vec3 GenerateLightDirDistribution(const glm::vec3& Normal, const glm::vec3& LightDir, float Roughness);

private:
	std::random_device mDevice;
	std::mt19937 mEngine;
	std::uniform_real_distribution<float> mDistribution;

private:
	float GetRandomFloat();
};
