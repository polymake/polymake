//author Benjamin Kutschan

#version 330

in vec4 vertices;

//in  vec3 tex_Coord;
out vec2 tex_Coord2;

void main(void)
{
	gl_Position = vertices - vec4(0.5, 0, 0, -1);
	
	tex_Coord2 = vec2(vertices.x, -vertices.y);
}