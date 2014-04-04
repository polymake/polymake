//author Benjamin Kutschan
//default line vertex shader
#version 330

uniform mat4 projection;
uniform mat4 modelview;

in vec4 vertex_coordinates;

in vec4 vertex_colors;
in vec4 edge_colors;

out vec4 edgeColor;
out vec4 vertexColor;

void main(void)
{
	gl_Position = projection * modelview * vertex_coordinates;
	edgeColor = edge_colors;
	vertexColor = vertex_colors;
}