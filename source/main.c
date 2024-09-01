#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "gl.h"
#include "noise.h"
#include "texture.h"
#include "report.h"
#include "float3.h"

GLFWwindow *new_window()
{
	// Basic window
	GLFWwindow *window = NULL;
	if (!glfwInit())
		return NULL;

	window = glfwCreateWindow(1920, 1080, "Tranquil", NULL, NULL);

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

	const GLubyte* renderer = glGetString(GL_RENDERER);
	printf("Renderer: %s\n", renderer);

	return window;
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

typedef struct {
	float3 eye;

	// Axial directions
	float3 front;
	float3 right;
	float3 up;

	float pitch;
	float yaw;
} camera_data;

static camera_data camera = {
	.eye = { 0, 0, 0 },
	.front = { 0, 0, 1 },
	.right = { 1, 0, 0 },
	.up = { 0, 1, 0 },
	.pitch = 0,
	.yaw = 0,
};

float clamp(float x, float min, float max)
{
	return fmin(max, fmax(min, x));
}

void camera_delta_yaw_pitch(camera_data *const camera, float dyaw, float dpitch)
{
	static const float BORDER_EPSILON = 0.001f;
	static const float BORDER_PITCH = M_PI_2 - BORDER_EPSILON;
	static const float3 UP = { 0, 1, 0 };

	camera->yaw += dyaw;
	camera->pitch = clamp(camera->pitch + dpitch, -BORDER_PITCH, BORDER_PITCH);

	camera->front = f3_spherical(1, camera->pitch, camera->yaw);
	camera->right = f3_cross(camera->front, UP);
	camera->up = f3_cross(camera->right, camera->front);
}

void gl_send_uniform_int(GLuint program, const char *name, int value)
{
	GLint location = glGetUniformLocation(program, name);
	return glUniform1i(location, value);
}

void gl_send_uniform_f3(GLuint program, const char *name, float3 value)
{
	GLint location = glGetUniformLocation(program, name);
	return glUniform3f(location, value.x, value.y, value.z);
}

void camera_send_uniforms(camera_data *const camera, GLuint program)
{
	gl_send_uniform_f3(program, "camera.eye", camera->eye);
	gl_send_uniform_f3(program, "camera.front", camera->front);
	gl_send_uniform_f3(program, "camera.right", camera->right);
	gl_send_uniform_f3(program, "camera.up", camera->up);
}

struct {
	float last_x;
	float last_y;
	bool initial;
	bool dragging;
} static mouse = {
	.initial = true,
	.dragging = false
};

void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS)
			mouse.dragging = true;
		else if (action == GLFW_RELEASE)
			mouse.dragging = false;
	}
}

void glfw_mouse_cursor_callback(GLFWwindow *window, double x, double y)
{
	static const float SENSIVITY = 0.001f;

	if (mouse.initial) {
		mouse.last_x = x;
		mouse.last_y = y;
		mouse.initial = false;
	}
	
	double dx = mouse.last_x - x;
	double dy = mouse.last_y - y;

	mouse.last_x = x;
	mouse.last_y = y;
	
	dx *= SENSIVITY;
	dy *= SENSIVITY;

	// Only drag when left mouse button is pressed
	if (mouse.dragging)
		camera_delta_yaw_pitch(&camera, dx, dy);
}

int main()
{
	GLFWwindow *window = new_window();

	// Window input callbacks
	glfwSetCursorPosCallback(window, glfw_mouse_cursor_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
	// glfwSetKeyCallback(window, keyboard_callback);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_debug_callback, NULL);

	file_info vertex_shader_source = read_file("shaders/screen.vert");
	file_info screen_shader_source = read_file("shaders/screen.frag");
	file_info blit_shader_source = read_file("shaders/blit.frag");

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

	printf("no error: %d\n", GL_NO_ERROR);
	printf("invalid enum: %d\n", GL_INVALID_ENUM);
	printf("invalid value: %d\n", GL_INVALID_VALUE);
	printf("invalid operation: %d\n", GL_INVALID_OPERATION);
	printf("invalid fb operation: %d\n", GL_INVALID_FRAMEBUFFER_OPERATION);
	printf("out of memory: %d\n", GL_OUT_OF_MEMORY);
	printf("gl stack underflow: %d\n", GL_STACK_UNDERFLOW);
	printf("gl stack overflow: %d\n", GL_STACK_OVERFLOW);

	gl_texture_create_info target_create_info = {
		.data = NULL,
		.width = 256,
		.height = 256,
		.internal = GL_RGB,
		.format = GL_RGB,
		.type = GL_UNSIGNED_INT,
		.filtering = GL_NEAREST
	};

	gl_texture target_texture = new_texture(target_create_info);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target_texture.ref, 0);

	GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, draw_buffers);

	noise_texture heightfield = perlin_noise(256, 256);
	
	gl_texture_create_info heightfield_create_info = {
		.data = heightfield.data,
		.width = heightfield.width,
		.height = heightfield.height,
		.internal = GL_R32F,
		.format = GL_RED,
		.type = GL_FLOAT,
		.filtering = GL_LINEAR
	};

	gl_texture heightfield_texture = new_texture(heightfield_create_info);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Primary rendering pass at pixelated resolution
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

		glViewport(0, 0, target_texture.width, target_texture.height);

		glUseProgram(linked_primary);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, heightfield_texture.ref);

		camera_send_uniforms(&camera, linked_primary);

		gl_send_uniform_int(linked_primary, "framebuffer.width", target_texture.width);
		gl_send_uniform_int(linked_primary, "framebuffer.height", target_texture.height);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
		// Blitting pass to presene the pixelated texture	
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);

		glUseProgram(linked_blitting);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, target_texture.ref);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glfwSwapBuffers(window);

		const float speed = 0.01f;
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			camera.eye = f3_fma(camera.eye, camera.up, speed);
		else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			camera.eye = f3_fma(camera.eye, camera.up, -speed);
		
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			camera.eye = f3_fma(camera.eye, camera.front, speed);
		else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camera.eye = f3_fma(camera.eye, camera.front, -speed);
		
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camera.eye = f3_fma(camera.eye, camera.right, speed);
		else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camera.eye = f3_fma(camera.eye, camera.right, -speed);
	}
}
