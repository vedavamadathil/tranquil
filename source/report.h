#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "gl.h"

void assert(bool cond, const char *fmt, ...)
{
	if (cond)
		return;

	va_list args;
	va_start(args, fmt);
	va_end(args);

	printf("[#] ");
	vprintf(fmt, args);
	fflush(stdout);

	abort();
}

void info(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	va_end(args);

	printf("[I] ");
	vprintf(fmt, args);
	fflush(stdout);
}

void gl_debug_callback
(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar *message,
	const void *user
)
{
	info("(GL) %s\n", message);
}