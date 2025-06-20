#version 440 core

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoords;

layout(location = 0) out vec2 vTexCoords;

void main()
{
	gl_Position = inPosition;

	vTexCoords = inTexCoords;
}
