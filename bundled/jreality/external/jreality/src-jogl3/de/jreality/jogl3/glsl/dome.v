//author Benjamin Kutschan

#version 330

in  vec3 in_Position;
in  vec3 tex_Coord;
out vec2 tex_Coord2;

void main(void)
{
	gl_Position = vec4(in_Position, 1.0);
	tex_Coord2 = tex_Coord.st;
}