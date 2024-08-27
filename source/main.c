#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "../glad/glad.h"
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

static inline int32_t fastfloor(float fp)
{
	int32_t i = (int32_t) fp;
	return (fp < i) ? (i - 1) : (i);
}

static const uint8_t perm[256] = {
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
    return perm[(uint8_t) i];
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

int main()
{
	GLFWwindow *window = new_window();

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
		.width = 512,
		.height = 512,
		.internal = GL_RGB,
		.format = GL_RGB,
		.type = GL_UNSIGNED_INT
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
		.type = GL_FLOAT
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

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
		// Blitting pass to presene the pixelated texture	
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);

		glUseProgram(linked_blitting);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, heightfield_texture.ref);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glfwSwapBuffers(window);
	}
}
