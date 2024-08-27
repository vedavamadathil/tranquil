#version 450

in vec2 uv;

uniform sampler2D framebuffer;

out vec4 fragment;

void main()
{
	fragment = texture(framebuffer, uv);
	// fragment = vec4(uv, 0, 1);
}