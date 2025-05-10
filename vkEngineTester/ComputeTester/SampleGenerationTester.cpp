#include "SampleGenerationTester.h"
#include <random>

static uint32_t sState = 37823;
static std::mutex sStateLock;

float GetRandom()
{
	std::lock_guard locker(sStateLock);

	sState *= sState * 747796405 + 2891336453;
	uint32_t result = (((sState >> (sState >> 28)) + 4) ^ sState) * 277803737;
	result = (result >> 22) ^ result;
	return static_cast<float>(result / 4294967295.0);
}

float GetRandomCPP()
{
	std::uniform_real_distribution distribution(0.0f, 1.0f);
	std::random_device device;

	std::mt19937 engine(device());

	return distribution(engine);
}

SampleGenerator::SampleGenerator()
	: mEngine(mDevice()), mDistribution(0.0f, 1.0f)
{
	auto TimeSinceEpoch = std::chrono::high_resolution_clock::now().time_since_epoch();
	size_t LoopCount = TimeSinceEpoch.count() % 37;

	for(size_t i = 0; i < LoopCount; i++)
		float Temp = GetRandom();
}

glm::vec3 SampleGenerator::GenerateUniformDistribution()
{
	float u = GetRandom();
	float v = GetRandom(); // Offset to get a different random number
	float phi = u * 2.0f * glm::pi<float>(); // Random azimuthal angle in [0, 2*pi]
	float cosTheta = 2.0f * (v - 0.5f); // Random polar angle, acos maps [0,1] to [0,pi]
	float sinTheta = glm::sqrt(1 - cosTheta * cosTheta);

	glm::vec3 sample;

	// Convert spherical coordinates to Cartesian coordinates
	sample.x = sinTheta * glm::cos(phi);
	sample.y = sinTheta * glm::sin(phi);
	sample.z = cosTheta;

	return sample;
}

glm::vec3 SampleGenerator::GenerateCosineWeightedDistribution(const glm::vec3 Normal)
{
	float u = GetRandom();
	float v = GetRandom(); // Offset to get a different random number
	float phi = u * 2.0f * glm::pi<float>(); // Random azimuthal angle in [0, 2*pi]
	float sqrtV = glm::sqrt(v);

	// Convert spherical coordinates to Cartesian coordinates

	glm::vec3 UnitVec;

	UnitVec.x = sqrtf(1.0f - v) * cosf(phi);
	UnitVec.y = sqrtf(1.0f - v) * sinf(phi);
	UnitVec.z = sqrtV;

	glm::vec3 Tangent = glm::abs(Normal.x) > glm::abs(Normal.z) ?
		glm::normalize(glm::vec3(Normal.z, 0.0, -Normal.x)) : glm::normalize(glm::vec3(0.0, -Normal.z, Normal.y));

	glm::vec3 Bitangent = glm::cross(Normal, Tangent);

	glm::mat3 TBN = glm::mat3(Tangent, Bitangent, Normal);

	return TBN * UnitVec;
}

glm::vec3 SampleGenerator::GenerateTrowbritzGGX_Distribution(const glm::vec3 Normal, float Roughness)
{
	float u = GetRandom();
	float v = GetRandom();

	float TanThetaSq = (Roughness * Roughness * u) / (1.0f - u);

	float SinTheta = glm::sqrt(TanThetaSq / (1.0f + TanThetaSq));
	float CosTheta = glm::sqrt(1.0f - SinTheta * SinTheta);

	float phi = 2.0f * glm::pi<float>() * v;

	glm::vec3 UnitVec;
	UnitVec.x = SinTheta * glm::cos(phi);
	UnitVec.y = SinTheta * glm::sin(phi);
	UnitVec.z = CosTheta;

	glm::vec3 Tangent = glm::abs(Normal.x) > glm::abs(Normal.z) ?
		glm::normalize(glm::vec3(Normal.z, 0.0, -Normal.x)) : glm::normalize(glm::vec3(0.0, -Normal.z, Normal.y));

	glm::vec3 Bitangent = glm::cross(Normal, Tangent);

	glm::mat3 TBN = glm::mat3(Tangent, Bitangent, Normal);

	return TBN * UnitVec;
}

glm::vec3 SampleGenerator::GenerateGGXVNDF_Distribution(const glm::vec3& View, const glm::vec3& Normal, float Roughness)
{
	float u1 = GetRandom();
	float u2 = GetRandom();

	glm::vec3 Tangent = glm::abs(Normal.x) > glm::abs(Normal.z) ?
		glm::normalize(glm::vec3(Normal.z, 0.0, -Normal.x)) : glm::normalize(glm::vec3(0.0, Normal.z, -Normal.y));

	glm::vec3 Bitangent = glm::cross(Normal, Tangent);

	glm::mat3 TBN = glm::mat3(Tangent, Bitangent, Normal);
	glm::mat3 TBNInv = glm::transpose(TBN);

	glm::vec3 ViewLocal = TBNInv * View;

	// Stretch view...
	glm::vec3 StretchedView = glm::normalize(glm::vec3(Roughness * ViewLocal.x,
		Roughness * ViewLocal.y, ViewLocal.z));

	// Orthonormal basis...

	glm::vec3 T1 = StretchedView.z < 0.999f ? glm::normalize(glm::cross(StretchedView, glm::vec3(0.0f, 0.0f, 1.0f))) :
		glm::vec3(1.0f, 0.0f, 0.0f);

	glm::vec3 T2 = glm::cross(T1, StretchedView);

	// Sample point with polar coordinates (r, phi)

	float a = 1.0f / (1.0f + StretchedView.z);
	float r = glm::sqrt(u1);
	float phi = (u2 < a) ? u2 / a * glm::pi<float>() : glm::pi<float>() * (1.0f + (u2 - a) / (1.0f - a));
	float P1 = r * glm::cos(phi);
	float P2 = r * glm::sin(phi) * ((u2 < a) ? 1.0f : StretchedView.z);

	// Form a normal

	glm::vec3 HalfVec = P1 * T1 + P2 * T2 + glm::sqrt(glm::max(0.0f, 1.0f - P1 * P1 - P2 * P2)) * StretchedView;

	// Unstretch normal

	HalfVec = glm::normalize(glm::vec3(Roughness * HalfVec.x, Roughness * HalfVec.y, glm::max(0.0f, HalfVec.z)));
	HalfVec = TBN * HalfVec;

	glm::vec3 Sample = glm::normalize(-View + 2.0f * dot(View, HalfVec) * HalfVec);

	return Sample;
}

glm::vec3 SampleGenerator::GenerateLightDirDistribution(const glm::vec3& Normal, 
	const glm::vec3& LightDir, float Roughness)
{
	const float scale = 9.0f;

	float u = GetRandom();
	float v = GetRandom();

	float SinhAlpha = glm::sinh(scale * Roughness);
	float AlphaInv = 1.0f / (scale * Roughness);

	float phi = 2.0f * v * glm::pi<float>();
	float cosAngle = AlphaInv * glm::asinh((2.0f * u - 1.0f) * SinhAlpha);

	// Convert spherical coordinates to Cartesian coordinates
	float x = sqrtf(1.0f - cosAngle * cosAngle) * cosf(phi);
	float y = sqrtf(1.0f - cosAngle * cosAngle) * sinf(phi);
	float z = cosAngle;

	glm::vec3 Tangent = glm::abs(LightDir.x) > glm::abs(LightDir.z) ?
		glm::normalize(glm::vec3(LightDir.z, 0.0f, -LightDir.x)) :
		glm::normalize(glm::vec3(0.0f, -LightDir.z, LightDir.y));

	glm::vec3 Bitangent = cross(LightDir, Tangent);

	glm::mat3 TBN = glm::mat3(Tangent, Bitangent, LightDir);

	glm::vec3 Sample = glm::normalize(TBN * glm::vec3(x, y, z));

	return glm::dot(Sample, Normal) < 0.0f ? -Sample : Sample;
	return Sample;
}

float SampleGenerator::GetRandomFloat()
{
	return mDistribution(mEngine);
}
