#ifndef RAY_TRACER_GLSL
#define RAY_TRACER_GLSL

#include "RayTracerConfig.glsl"
#include "Collisions.glsl"
#include "Shading.glsl"
#include "Sampler.glsl"

const vec3 MaxRadiance = vec3(100.0);

#define STACK_SIZE                20

vec3 GlobalIllumination(in StackEntry[STACK_SIZE] stack, int depth, bool foundLightSrc)
{
	// The last entry is always assumed to be a light source or a fragment on a skybox
	if (depth < 0)
		return vec3(0.0);

	uint TransmissionCount = 0;

	StackEntry LastEntry = stack[depth--];
	StackEntry SecondLastEntry = stack[depth--];

	BSDF_Input bsdfInput;
	
	bsdfInput.Normal = SecondLastEntry.HitInfo.Normal;
	bsdfInput.LightDir = SecondLastEntry.IncomingRay.Direction;
	bsdfInput.HalfVec = SecondLastEntry.HalfVec;
	
	bsdfInput.RefractiveIndexRatio = SecondLastEntry.InterfaceRefractiveIndex;
	
	bsdfInput.InterfaceRoughness = SecondLastEntry.HitInfo.ObjectMaterial.Roughness;
	bsdfInput.InterfaceMetallic = SecondLastEntry.HitInfo.ObjectMaterial.Metallic;
	bsdfInput.TransmissionWeight = SecondLastEntry.HitInfo.ObjectMaterial.Transmission;
	bsdfInput.BaseColor = SecondLastEntry.HitInfo.ObjectMaterial.Albedo;

	float Hemisphere = dot(bsdfInput.LightDir, bsdfInput.Normal) > 0.0 ? 0.0 : 1.0;
	
	// Debug variable...
	TransmissionCount += Hemisphere > 0.5 ? 1 : 0;

	vec3 ViewDir = -SecondLastEntry.OutgoingRay.Direction;

	vec3 LightSrcColor = foundLightSrc ? LastEntry.HitInfo.LightProps.Color :
		SampleCubeMap(bsdfInput.LightDir, 0.0);

	// Direct light
	vec3 Radiance = CookTorranceBSDF(bsdfInput, Hemisphere);

	Radiance *= LightSrcColor;
	Radiance /= SecondLastEntry.IncomingSampleWeight;

	Radiance = min(Radiance, MaxRadiance);

	// Indirect light
	while (depth >= 0)
	{
		StackEntry Entry = stack[depth--];

		bsdfInput.Normal = Entry.HitInfo.Normal;
		bsdfInput.LightDir = Entry.IncomingRay.Direction;
		bsdfInput.HalfVec = Entry.HalfVec;
		
		bsdfInput.RefractiveIndexRatio = Entry.InterfaceRefractiveIndex;
		
		bsdfInput.InterfaceRoughness = Entry.HitInfo.ObjectMaterial.Roughness;
		bsdfInput.InterfaceMetallic = Entry.HitInfo.ObjectMaterial.Metallic;
		bsdfInput.TransmissionWeight = Entry.HitInfo.ObjectMaterial.Transmission;
		bsdfInput.BaseColor = Entry.HitInfo.ObjectMaterial.Albedo;

		Hemisphere = dot(bsdfInput.LightDir, bsdfInput.Normal) > 0.0 ? 0.0 : 1.0;
		
		// Debug variable...
		TransmissionCount += Hemisphere > 0.5 ? 1 : 0;

		ViewDir = -Entry.OutgoingRay.Direction;

		// Calculate the outgoing Radiance from the radiance carried by the previous ray
		vec3 NewRadiance = CookTorranceBSDF(bsdfInput, Hemisphere);

		NewRadiance *= Radiance;
		// Weight the results according to the sample distributions...
		NewRadiance /= Entry.IncomingSampleWeight;
		Radiance = NewRadiance;

		Radiance = min(Radiance, MaxRadiance);
	}

	Radiance = min(Radiance, MaxRadiance);

	//if(TransmissionCount % 2 != 0)
	//	return vec3(100, 100, 0);

	return Radiance;
}

vec3 TraceRayMonteCarlo(in Ray ray)
{
	StackEntry stack[STACK_SIZE];
	int stackPtr = 0;

	float RayRefractiveIndex[STACK_SIZE];
	int layerPtr = 0;

	CollisionInfo hitInfo;
	CheckForRayCollisions(hitInfo, ray);

	if(!hitInfo.HitOccured)
		return SampleCubeMap(ray.Direction, 0.0);

	if(hitInfo.IsLightSrc)
		return hitInfo.LightProps.Color * hitInfo.NormalInverted;

	stack[stackPtr].HitInfo = hitInfo;
	stack[stackPtr].OutgoingRay = ray;

	RayRefractiveIndex[layerPtr] = 1.0;

	bool FoundLightSrc = false;
	uint i = 0;

	// Include the light src if we find one and then terminate
	for(; i < uSceneInfo.MaxBounceLimit; i++)
	{
		layerPtr -= int((-hitInfo.NormalInverted + 1.0) / 2.0 + 0.5);// < 0.0 ? 1 : 0;
		float RefractiveIndex = RayRefractiveIndex[layerPtr] / hitInfo.ObjectMaterial.RefractiveIndex;

		RefractiveIndex = pow(RefractiveIndex, hitInfo.NormalInverted);

		if(!SampleUnitVec(stack[stackPtr], RefractiveIndex))
			return vec3(0.0, 0.0, 0.0);

		RayRefractiveIndex[++layerPtr] = hitInfo.ObjectMaterial.RefractiveIndex;

		layerPtr -= dot(stack[stackPtr].IncomingRay.Direction, hitInfo.Normal) *
			hitInfo.NormalInverted > 0.0 ? 1 : 0;

		CheckForRayCollisions(hitInfo, stack[stackPtr].IncomingRay);
		stackPtr++;

		stack[stackPtr].HitInfo = hitInfo;
		stack[stackPtr].OutgoingRay = stack[stackPtr - 1].IncomingRay;

		if(!hitInfo.HitOccured || hitInfo.IsLightSrc)
		{
			FoundLightSrc = hitInfo.IsLightSrc;
			break;
		}
	}

	vec3 IncomingLight = (i != uSceneInfo.MaxBounceLimit || FoundLightSrc) ?
		GlobalIllumination(stack, stackPtr, FoundLightSrc) : vec3(0.0);

	return IncomingLight;
}

#endif