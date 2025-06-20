import DiffuseBSDF

layout(set = 2, binding = 0) uniform sampler2D uTexture;

SampleInfo Evaluate(in Ray ray, in CollisionInfo collisionInfo)
{
	DiffuseBSDF_Input diffuseInput;
	diffuseInput.ViewDir = -ray.Direction;
	diffuseInput.Normal = collisionInfo.Normal;
	diffuseInput.BaseColor = vec3(0.6, 0.6, 0.6);

	Face face = sFaces[collisionInfo.PrimitiveID];

	vec2 tex = GetTexCoords(face, collisionInfo);

	diffuseInput.BaseColor = texture(uTexture, tex).rgb;
	//diffuseInput.BaseColor = vec3(0.6);

	SampleInfo sampleInfo = SampleDiffuseBSDF(diffuseInput);

	sampleInfo.Luminance = DiffuseBSDF(diffuseInput, sampleInfo);

	//sampleInfo.Weight = 1.0;
	//sampleInfo.Luminance = vec3(0.6, 0.0, 0.6);

	return sampleInfo;
}
