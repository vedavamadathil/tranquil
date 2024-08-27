#version 450

in vec2 uv;

layout (location = 0) out vec4 fragment;

void main()
{
	fragment = vec4(uv, 0, 1);
}