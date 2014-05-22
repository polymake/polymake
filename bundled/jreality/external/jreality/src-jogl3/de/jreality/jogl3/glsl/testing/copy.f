//author Benjamin Kutschan
//default polygon fragment shader
#version 150

out vec4 glFragColor;
smooth in vec4 texCoord;

uniform sampler2D image;
//uniform sampler2D image0;
//uniform sampler2D image1;
//uniform sampler2D image2;
//uniform sampler2D image3;

void main(void)
{
	glFragColor = texture(image, texCoord.st);
	//vec4 color = texture( image0, texCoord.st);
	//color += texture( image1, texCoord.st);
	//color += texture( image2, texCoord.st);
	//color += texture( image3, texCoord.st);
	//color = vec4(color.r/4, color.g/4, color.b/4, 1);
	//glFragColor = color;
}
