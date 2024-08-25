project('tranquil', 'c')

glfw = dependency('glfw3')

glad = include_directories('glad/include')

executable('tranquil',
	'main.c',
	'glad/src/glad.c',
	include_directories: glad,
	dependencies: glfw)
