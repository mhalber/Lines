# Lines

<p align="center"> 
<img src="screenshots/aa_lines.png">
</p>

## What?

This repository explores different ways of rendering wide lines using OpenGL. Rather than a library, it is meant to be a
number of reference implementations to produce thick, antialiased lines in OpenGL. Currently there are 5 implementations
availabe in this repo:

1. OpenGL lines - using glLineWidth(...) functionality
2. CPU lines - extending lines to quads on the CPU.
3. Geometry Shaders - extending lines to quads on GPU during Geometry Shader stage.
4. Instancing Lines - hijacing the instancing functionality to render a number of line segments by repeating instanced quad.
5. Texture buffer lines - using combination of a texture that stores line data and gl_VertexID to generate quads in Vertex Shader.

## Why?

The reason for exploring different implementations of wide-line rendering is steming from the fact that using build-in
OpenGL functionality for this task is very limited. While combining glLineWidth(...) and glEnable(GL_LINE_SMOOTH) 'can'
be used to produce antialiased lines, there are a number of issues.

- No guarantee that this approach will work. Implementations of OpenGL vary across systems and the glGet(GL_LINE_WIDTH_RANGE)
On my system (NVidia GTX 1070 Max-Q) I am able to use such functionality, but it might not be true for you.
- Limited range. As stated above the glGet line width will return supported range - creating lines wider or thinner than 
such range is not supported. For example using implementations 1 vs other four will produce following images when asking for
a wide range:

- no control over AA

- no control over line width within a drawcall.

## Methods
Here we briefly discuss how each method is implemented.



