//author Benjamin Kutschan
//default polygon cubemap shader
#version 330

in vec4 vertex_coordinates;
in vec2 tex_coords;

out vec2 texCoord;

void main(void)
{
	texCoord = tex_coords;
	gl_Position = vertex_coordinates;
}