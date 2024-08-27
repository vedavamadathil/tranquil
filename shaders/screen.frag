#version 450

// Screen space coordinate
in vec2 uv;

// Framebuffer information
struct framebuffer_data {
	int width;
	int height;
};

uniform framebuffer_data framebuffer;

// Camera information
struct camera_data {
	vec3 eye;
	vec3 front;
	vec3 right;
	vec3 up;
};

uniform camera_data camera;

// Ray for intersection
struct ray {
	vec3 origin;
	vec3 direction;
};

// Ray generation
const float PI = 3.141592653589793238462643383279502884197;

ray pixel_ray()
{
	const float fov = 45.0;

	float rad_fov = fov * PI/180.0f;
	float scale = tan(rad_fov * 0.5f);
	float aspect = float(framebuffer.width) / float(framebuffer.height);

	vec2 cuv = (1.0f - 2.0f * uv) * vec2(scale * aspect, scale);
	vec3 direction = normalize(camera.right * cuv.x - camera.up * cuv.y + camera.front);

	return ray(camera.eye, direction);
}

// Heightfield for the base plane
layout (binding = 0) uniform sampler2D heightfield;

layout (location = 0) out vec4 fragment;

vec3 sky_color(inout ray r)
{
    return exp(-(r.direction.y + 0.1) / vec3(0.1, 0.3, 0.6));
}

void main()
{
	ray r = pixel_ray();
	fragment = vec4(sky_color(r), 1);
}