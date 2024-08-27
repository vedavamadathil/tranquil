#pragma once

#include "gl.h"

typedef struct {
	GLuint ref;
	int width;
	int height;
} gl_texture;

typedef struct {
	void *data;
	int width;
	int height;
	GLuint internal;
	GLuint format;
	GLuint type;
} gl_texture_create_info;

gl_texture new_texture(gl_texture_create_info info)
{
	gl_texture result;
	result.width = info.width;
	result.height = info.height;

	glGenTextures(1, &result.ref);
	glBindTexture(GL_TEXTURE_2D, result.ref);
	glTexImage2D(GL_TEXTURE_2D, 0, info.internal, info.width, info.height, 0, info.format, info.type, info.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	return result;
}