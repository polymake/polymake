//author Benjamin Kutschan
//default polygon cubemap shader
#version 330

in vec4 vertex_coordinates;
in vec4 vertex_colors;

out vec4 color;

void main(void)
{
	color = vertex_colors;
	gl_Position = vertex_coordinates;
}