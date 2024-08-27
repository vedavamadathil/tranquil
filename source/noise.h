#pragma once

#include <stdlib.h>
#include <stdint.h>

static inline int32_t fastfloor(float fp)
{
	int32_t i = (int32_t) fp;
	return (fp < i) ? (i - 1) : (i);
}

static const uint8_t permutation[256] = {
	151, 160, 137, 91, 90, 15,
	131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
	190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
	77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
	102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
	135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
	5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
	223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
	251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
	49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
	138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

static inline uint8_t hash(int32_t i)
{
    return permutation[(uint8_t) i];
}

static float grad(int32_t hash, float x, float y)
{
    const int32_t h = hash & 0x3F;
    const float u = h < 4 ? x : y;
    const float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float perlin_simplex_noise(float x, float y)
{
	float n0;
	float n1;
	float n2;

	static const float F2 = 0.366025403f;
	static const float G2 = 0.211324865f;

	const float s = (x + y) * F2;
	const float xs = x + s;
	const float ys = y + s;
	const int32_t i = fastfloor(xs);
	const int32_t j = fastfloor(ys);

	const float t = (float) (i + j) * G2;
	const float X0 = i - t;
	const float Y0 = j - t;
	const float x0 = x - X0;
	const float y0 = y - Y0;

	int32_t i1, j1;
	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	} else {
		i1 = 0;
		j1 = 1;
	}

	const float x1 = x0 - i1 + G2;
	const float y1 = y0 - j1 + G2;
	const float x2 = x0 - 1.0f + 2.0f * G2;
	const float y2 = y0 - 1.0f + 2.0f * G2;

	const int gi0 = hash(i + hash(j));
	const int gi1 = hash(i + i1 + hash(j + j1));
	const int gi2 = hash(i + 1 + hash(j + 1));

	float t0 = 0.5f - x0*x0 - y0*y0;
	if (t0 < 0.0f) {
		n0 = 0.0f;
	} else {
		t0 *= t0;
		n0 = t0 * t0 * grad(gi0, x0, y0);
	}

	float t1 = 0.5f - x1*x1 - y1*y1;
	if (t1 < 0.0f) {
		n1 = 0.0f;
	} else {
		t1 *= t1;
		n1 = t1 * t1 * grad(gi1, x1, y1);
	}

	float t2 = 0.5f - x2*x2 - y2*y2;
	if (t2 < 0.0f) {
		n2 = 0.0f;
	} else {
		t2 *= t2;
		n2 = t2 * t2 * grad(gi2, x2, y2);
	}

	float n = 45.23065f * (n0 + n1 + n2);
	
	return 0.5f * (n + 1);
}

typedef struct {
	float *data;
	int width;
	int height;
} noise_texture;

noise_texture perlin_noise(int width, int height)
{
	noise_texture result;
	result.data = (float *) malloc(width * height * sizeof(float));
	result.width = width;
	result.height = height;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float noise = 0;
			noise += perlin_simplex_noise((float) x / width, (float) y / height);
			noise += 0.5 * perlin_simplex_noise((float) 2 * x / width, (float) 2 * y / height);
			noise += 0.25 * perlin_simplex_noise((float) 4 * x / width, (float) 4 * y / height);
			noise += 0.125 * perlin_simplex_noise((float) 8 * x / width, (float) 8 * y / height);
			noise += 0.0625 * perlin_simplex_noise((float) 16 * x / width, (float) 16 * y / height);
			noise += 0.03125 * perlin_simplex_noise((float) 32 * x / width, (float) 32 * y / height);
			result.data[x + y * width] = noise/2;
		}
	}

	return result;
}