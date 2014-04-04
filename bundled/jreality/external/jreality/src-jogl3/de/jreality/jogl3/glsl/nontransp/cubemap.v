//author Benjamin Kutschan
//default polygon cubemap shader
#version 330

uniform mat4 projection;
uniform mat4 modelview;
uniform float scale;

in vec4 vertex_coordinates;
in vec3 vertex_texturecoordinates;
in float vertex_tex_no;

out vec3 texCoord;
out float X;

void main(void)
{
	X = vertex_tex_no;
	texCoord = vertex_texturecoordinates;
	vec4 coords = vec4(scale*vertex_coordinates.xyz, 1);
	gl_Position = projection * modelview * coords;
}