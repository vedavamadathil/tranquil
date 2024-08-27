#version 450

vec4[4] vertex_bank = vec4[4]
(
	vec4( 1,  1, 0, 1),
	vec4( 1, -1, 0, 1),
	vec4(-1, -1, 0, 1),
	vec4(-1,  1, 0, 1)
);

vec2[4] uv_bank = vec2[4]
(
	vec2(1, 1),
	vec2(1, 0),
	vec2(0, 0),
	vec2(0, 1)
);

out vec2 uv;

void main()
{
	gl_Position = vertex_bank[gl_VertexID];
	uv = uv_bank[gl_VertexID];
}