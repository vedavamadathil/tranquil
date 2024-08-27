project('tranquil', 'c')

cc = meson.get_compiler('c')
math = cc.find_library('m', required : false)
glfw = dependency('glfw3')

executable('tranquil',
	'source/main.c',
	'glad/glad.c',
	dependencies: [ glfw, math ])
