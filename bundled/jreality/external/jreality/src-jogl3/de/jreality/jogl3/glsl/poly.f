//author Benjamin Kutschan
//default polygon fragment shader
#version 330

out vec4 glFragColor;
uniform vec4 diffuseColor;
//in vec2 gl_PointCoord;
void main(void)
{
	//vec4 color2 = diffuseColor; //vec4(1, 0, 0, 1);
	//glFragColor = vec4(color2.rgb * lightInflux, 1.0);
	glFragColor = vec4(1, 0, 0, 1);
}