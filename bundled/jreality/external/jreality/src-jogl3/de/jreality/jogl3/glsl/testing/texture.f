//author Benjamin Kutschan
//default polygon fragment shader
#version 150

out vec4 glFragColor;
smooth in vec4 texCoord;

uniform sampler2D image;

void main(void)
{
	glFragColor = texture(image, texCoord.st);
}
