//author Benjamin Kutschan
//default polygon cubemap shader
#version 330

out vec4 glFragColor;
uniform sampler2D image;
in vec2 texCoord;
void main(void)
{	
	glFragColor = texture(image, texCoord);
}