#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

GLFWwindow *new_window()
{
	// Basic window
	GLFWwindow *window = NULL;
	if (!glfwInit())
		return NULL;

	window = glfwCreateWindow(800, 600, "Tranquil", NULL, NULL);

	// Check if window was created
	if (!window) {
		glfwTerminate();
		return NULL;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Load OpenGL functions using GLAD
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		printf("Failed to initialize OpenGL context\n");
		return NULL;
	}

	// // Set up callbacks
	// glfwSetCursorPosCallback(window, mouse_callback);
	// glfwSetMouseButtonCallback(window, mouse_button_callback);
	// glfwSetKeyCallback(window, keyboard_callback);

	const GLubyte* renderer = glGetString(GL_RENDERER);

	printf("Renderer: %s\n", renderer);
	return window;
}

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

typedef struct {
	const char *path;
	const char *content;
} file_info;

file_info read_file(const char *path)
{
	FILE *file = fopen(path, "r");
	assert(file, "could not open file: '%s'\n", path);

        int length;
        fseek(file, 0, SEEK_END);
	length  = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buffer = (char *) malloc(length + 1);
	memset(buffer, 0, length + 1);
	fread(buffer, 1, length, file);
        fclose(file);

	length = strlen(path);
	char *name = (char *) malloc(length + 1);
	memcpy(name, path, length + 1);

        file_info fi = {
		.path = name,
		.content = buffer,
	};

	return fi;
}

GLuint compile_shader_source(GLuint shader_stage, file_info fi)
{
	unsigned int shader = glCreateShader(shader_stage);
	glShaderSource(shader, 1, &fi.content, NULL);
	glCompileShader(shader);

	// Check for errors
	int status;
	char log[512];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (!status) {
		glGetShaderInfoLog(shader, 512, NULL, log);
		assert(false, "failed to compile shader (%s):\n%s\n", fi.path, log);
	}

	info("successfully compiled shader (%s)\n", fi.path);

	return shader;
}

bool link_program(GLuint program)
{
	int status;
	char log[512];

	glLinkProgram(program);

	// Check if program linked successfully
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (!status) {
		glGetProgramInfoLog(program, 512, NULL, log);
		assert(false, "failed to link program: %s\n", log);
		return false;
	}

	return true;
}

void opengl_sift_errors()
{
	GLuint error;
	while ((error = glGetError()) != GL_NO_ERROR)
		info("error status: %d\n", error);
}

typedef struct {
	GLuint ref;
	int width;
	int height;
} gl_texture;

typedef struct {
	int width;
	int height;
} gl_texture_create_info;

gl_texture new_texture(gl_texture_create_info info)
{
	gl_texture result;
	result.width = info.width;
	result.height = info.height;

	glGenTextures(1, &result.ref);
	glBindTexture(GL_TEXTURE_2D, result.ref);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, info.width, info.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	return result;
}

int main()
{
	GLFWwindow *window = new_window();

	file_info vertex_shader_source = read_file("../screen.vert");
	file_info screen_shader_source = read_file("../screen.frag");
	file_info blit_shader_source = read_file("../blit.frag");

	GLuint vertex_shader = compile_shader_source(GL_VERTEX_SHADER, vertex_shader_source);
	GLuint screen_shader = compile_shader_source(GL_FRAGMENT_SHADER, screen_shader_source);
	GLuint blit_shader = compile_shader_source(GL_FRAGMENT_SHADER, blit_shader_source);

	GLuint linked_primary = glCreateProgram();
	glAttachShader(linked_primary, vertex_shader);
	glAttachShader(linked_primary, screen_shader);
	assert(link_program(linked_primary), "");
	
	GLuint linked_blitting = glCreateProgram();
	glAttachShader(linked_blitting, vertex_shader);
	glAttachShader(linked_blitting, blit_shader);
	assert(link_program(linked_blitting), "");

	GLuint framebuffer = 0;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	opengl_sift_errors();

	printf("no error: %d\n", GL_NO_ERROR);
	printf("invalid enum: %d\n", GL_INVALID_ENUM);
	printf("invalid value: %d\n", GL_INVALID_VALUE);
	printf("invalid operation: %d\n", GL_INVALID_OPERATION);
	printf("invalid fb operation: %d\n", GL_INVALID_FRAMEBUFFER_OPERATION);
	printf("out of memory: %d\n", GL_OUT_OF_MEMORY);
	printf("gl stack underflow: %d\n", GL_STACK_UNDERFLOW);
	printf("gl stack overflow: %d\n", GL_STACK_OVERFLOW);

	gl_texture_create_info target_create_info = {
		.width = 512,
		.height = 512,
	};

	gl_texture target = new_texture(target_create_info);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target.ref, 0);

	GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, draw_buffers);

	opengl_sift_errors();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Primary rendering pass at pixelated resolution
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		glViewport(0, 0, target.width, target.height);

		glUseProgram(linked_primary);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
		opengl_sift_errors();

		// Blitting pass to presene the pixelated texture	
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);

		glUseProgram(linked_blitting);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, target.ref);

		glUniform1i(glGetUniformLocation(linked_blitting, "framebuffer"), 0);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
		opengl_sift_errors();

		glfwSwapBuffers(window);
	}
}
