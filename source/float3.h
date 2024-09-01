#pragma once

#include <math.h>

typedef struct {
	float x;
	float y;
	float z;
} float3;

float3 f3(float x, float y, float z)
{
	float3 v = { x, y, z };
	return v;
}

float3 f3_spherical(float r, float phi, float theta)
{
	float3 v;
	v.x = r * cos(phi) * cos(theta);
	v.y = r * sin(phi);
	v.z = r * cos(phi) * sin(theta);
	return v;
}

float3 f3_cross(float3 a, float3 b)
{
	float3 c;
	c.x = a.y * b.z - a.z * b.y;
	c.y = a.z * b.x - a.x * b.z;
	c.z = a.x * b.y - a.y * b.x;
	return c;
}

float3 f3_fma(float3 a, float3 b, float k)
{
	float3 c;
	c.x = a.x + b.x * k;
	c.y = a.y + b.y * k;
	c.z = a.z + b.z * k;
	return c;
}