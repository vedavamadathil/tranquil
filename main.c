#include <stdio.h>

#include <glad/glad.h>
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
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
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

int main()
{
	GLFWwindow *window = new_window();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}
