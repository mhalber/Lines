# WORK IN PROGRESS
# Lines

<p align="center"> 
<img src="screenshots/aa_lines.png">
</p>

## What?

This repository explores different ways of rendering wide lines using OpenGL. Rather than a library, it is meant to be a
number of reference implementations to produce thick, antialiased lines in OpenGL. Currently there are 5 implementations
availabe in this repo:

1. OpenGL lines - using `glLineWidth(width_value)` functionality.
2. CPU lines - extending lines to quads on the CPU.
3. Geometry Shaders - extending lines to quads on GPU during Geometry Shader stage.
4. Instancing Lines - hijacing the instancing functionality to render a number of line segments by repeating instanced quad.
5. Texture buffer lines - using combination of a texture that stores line data and gl_VertexID to generate quads in Vertex Shader.

For simplicity, the implementation assume vertex buffer where each pair of points creates a line segment (`GL_LINES` behavior)

## Why?

The reason for exploring different implementations of wide line rendering is steming from the fact that using the build-in
OpenGL functionality for this task is very limited, if working at all. 
While combining `glLineWidth(width_value)` and `glEnable(GL_LINE_SMOOTH)` **"can"** be used to produce antialiased lines, there are a number of issues:

- No guarantee that this approach will work. Implementations of OpenGL vary across systems - the `glGet( GL_LINE_WIDTH_RANGE, range)` can simply tell you that your implementation only supports a range (1.0,1.0], leading to effectively no control over 
the line width. On my system at the time of writing (NVidia GTX 1070 Max-Q, OpenGL 4.5, driver 441.08) the supported `GL_LINE_WIDTH_RANGE` is [1.0, 10.0]. As such I am able to produce image below, but your milage may vary:

- Limited range. As stated above the `glGet( GL_LINE_WIDTH_RANGE, ...)` line width will return supported range - creating lines with widths outside this range will fail. For example, using implementations 1 vs other four will produce following images when
asking to draw 3 lines segments with widths of 10, 15, 20 pixels:

% figure

- No control over AA. `glEnable(GL_LINE_SMOOTH)` enables anti-aliasing, however we have no control over this behavior. In contrast
the custom implementation in this repository (2-5) allow user to specify the width of the smoothed region:

% figure

- No control over line width within a drawcall. Due to the nature of `glLineWidth(width_value)` the line width
need to constant within a drawcall. This means multiple drawcalls needs to be issue to render lines with varying widths.
In contrast, the custom implementation expose line width as a per-vertex attribute, allowing to render variable-width
lines with a single draw call.

## Methods

Here we briefly discuss how each method is implemented. Main idea is to have a parametrized quad and smooth the edge regions.

### CPU lines
If everything else fails, we can always just take the input buffer and use it to calculate new vertex locations.

### Geometry Shader lines
We can utilize the geometry shaders available in OpenGL 3.3+ to produce the desired vertex data on GPU

### Instancing lines

% Add references

### Texture Buffer lines

% Add references to Chrisoph Kubish talk on the OpenGL blueprints.




## Method Comparison



