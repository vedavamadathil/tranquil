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

vec3 sky_color(vec3 direction)
{
    return exp(-(direction.y + 0.1) / vec3(0.1, 0.3, 0.6));
}

const float out_of_bounds = -1e6;

float heightfield_evaluate(vec2 uv)
{
	// TODO: chunking approach
	if (uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1)
		return out_of_bounds;

	return texture(heightfield, uv).r;
}

// Assuming chunk bounds are [0, 1] x [0, 1]
// and also that the ray origin is out of bounds
float time_to_chunks(vec3 origin, vec3 direction)
{
	vec2 p = origin.xz;
	vec2 d = direction.xz;

	// Running ray-x-aabb intersection to find the time
	vec2 idir = 1/d;
	vec2 tbot = idir * (vec2(0) - p);
	vec2 ttop = idir * (vec2(1) - p);
	vec2 tmin = min(ttop, tbot);
	vec2 tmax = max(ttop, tbot);
	float t0 = max(tmin.x, tmin.y);
	float t1 = min(tmax.x, tmax.y);

	if (t0 < 0 || t1 < t0)
		return -1;

	return t0;
}

struct hit_info {
	vec3 position;
	vec3 normal;
	float t;
};

void raytrace(out hit_info info, inout ray r)
{
	const int maxit = 1000;

	// Default to no intersection
	info.t = -1;

	float dt = 0.01;

	float ph = 0;
	float t = 0;
	int i = 0;

	while (t < 10 && (i++) < maxit) {
		vec3 s = r.origin + r.direction * t;

		float h = heightfield_evaluate(s.xz);
		if (h <= -1e3) {
			float tt = time_to_chunks(s, r.direction);
			if (tt < 0)
				return;

			t += tt + dt;
			continue;
		}

		if (h > s.y) {
			float dh = h - ph;
			vec3 df = -camera.front * dh;
			vec3 du = vec3(0, dt, 0);

			info.position = s;
			info.normal = normalize(du + df);
			info.t = t;
			return;
		}

		t += dt;
		ph = h;
	}
}

void main()
{
	ray r = pixel_ray();

	hit_info info;
	raytrace(info, r);

	if (info.t >= 0) {
		vec3 fog = vec3(1 - exp(-info.t));
		fragment = vec4(fog, 1);

		// fragment = vec4(info.normal * 0.5 + 0.5, 1);

		// r.origin = info.position + 0.01 * info.normal;
		// r.direction = vec3(1, 1, 1);

		// raytrace(info, r);
		// if (info.t >= 0)
		// 	fragment = vec4(vec3(0.1), 1);
	} else {
		fragment = vec4(sky_color(r.direction), 1);
	}
}