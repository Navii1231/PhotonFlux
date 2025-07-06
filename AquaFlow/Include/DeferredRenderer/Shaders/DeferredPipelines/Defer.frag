#version 440 core

layout(location = 0) out vec4 oPosition;
layout(location = 1) out vec4 oNormal;
layout(location = 2) out vec4 oTangent;
layout(location = 3) out vec4 oBitangent;
layout(location = 4) out vec2 oTexCoords;
layout(location = 5) out vec2 oMaterialIDs;

layout(set = 0, binding = 0) uniform CameraInfo
{
	mat4 uProjection;
	mat4 uView;
};

// Receving data to fragment shader
layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec4 vNormal;
layout (location = 2) in vec4 vTangent;
layout (location = 3) in vec4 vBitangent;
layout (location = 4) in vec3 vTexCoords;
layout (location = 5) in vec2 vMatID_MeshID;

void main()
{
	oPosition = vPosition;
	oNormal = vNormal;
	oTangent = vTangent;
	oBitangent = vBitangent;
	oTexCoords = vTexCoords.xy;
	oMaterialIDs = vMatID_MeshID;
}