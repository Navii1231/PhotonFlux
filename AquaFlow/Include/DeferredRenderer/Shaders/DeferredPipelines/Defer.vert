#version 440 core

layout(location = 0) in vec4 iPosition;
layout(location = 1) in vec4 iNormal;
layout(location = 2) in vec4 iTangent;
layout(location = 3) in vec4 iBitangent;
layout(location = 4) in vec3 iTexCoords;
layout(location = 5) in vec2 iMatID_MeshID; // Holds material id and mesh id

layout(set = 0, binding = 0) uniform CameraInfo
{
	mat4 uProjection;
	mat4 uView;
};

layout(std430, set = 0, binding = 1) buffer ModelMatrices
{
	mat4 sModels[];
};

// Sending data to fragment shader
layout (location = 0) out vec4 vPosition;
layout (location = 1) out vec4 vNormal;
layout (location = 2) out vec4 vTangent;
layout (location = 3) out vec4 vBitangent;
layout (location = 4) out vec3 vTexCoords;
layout (location = 5) out vec2 vMatID_MeshID;

void main()
{
	uint MeshID = uint(iMatID_MeshID.g + 0.5);

	vec4 VertexPos = uView * sModels[MeshID] * iPosition;

	gl_Position = uProjection * VertexPos;

	vPosition = VertexPos;
	vNormal = uView * iNormal;
	vTangent = uView * iTangent;
	vBitangent = uView * iBitangent;
	vTexCoords = iTexCoords;
	vMatID_MeshID = iMatID_MeshID;
}
