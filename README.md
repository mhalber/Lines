# Lines

<p align="center"> 
<img src="screenshots/aa_lines.png">
</p>

## What?

This repository explores different ways of rendering wide lines using OpenGL. Rather than  a library, it is meant to be a
number of reference implementations to produce thick, antialiased lines in OpenGL. Currently there are 4 implementations:

1. CPU lines - extending lines to quads on cpu.
2. Geometry Shaders - extending lines to quads on GPU during Geometry Shader stage
3. Instancing Lines - hijacing the instancing functionality to render a number of line segments by repeating instanced quad
4. Texture buffer lines - using combination of a texture that stores line data and gl_VertexID to generate quads in Vertex Shader 

Additinally, there is an implementation using GL_LINES with glLineWidth(...)

## Why?

Wide lines are not uniformly supported in native OpenGL. Technically, the line width can be changed with calls to 
glLineWidth(...), the implementation can specify GL_LINE_WIDTH_RANGE and GL_ALIASED_LINE_WIDTH_RANGE to be between 1.0 and 1.0.
This means that any call to glLineWidth with a value different than 1.0 will be ignored. Second issue comes from the complete
lack of control over antialiasing of the line - user can request opengl to render antialiased lines by glEnable(GL_LINE_SMOOTH),
however there is no control over how strong the blurring of the edge will be. Lastly, there is no way to have
a line segment with a varying width across.

As such, we would like to have an alternative that alleviates above issues.

## Methods



