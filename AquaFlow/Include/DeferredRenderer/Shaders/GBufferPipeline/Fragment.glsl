#version 440 core

layout(location = 0) out vec4 vPosition;
layout(location = 1) out vec4 vNormal;
layout(location = 2) out vec4 vTangent;
layout(location = 3) out vec4 vBitangent;
layout(location = 4) out vec2 vTexCoords;
layout(location = 5) out vec2 vMaterialIDs;

layout(set = 0, binding = 1) uniform CameraInfo
{
	mat4 uProjection;
	mat4 uView;
};

// Sending data to fragment shader
in struct
{
	vec4 iPosition;
	vec4 iNormal;
	vec4 iTangent;
	vec4 iBitangent;
	vec2 iTexCoords;
	vec2 iMatID_MeshID;
};

void main()
{
	vPosition = iPosition
	vNormal = iNormal
	vTangent = iTangent
	vBitangent = iBitangent
	vTexCoords = iTexCoords
	vMatID_MeshID = iMatID_MeshID
}