//author Benjamin Kutschan
//default polygon fragment shader
#version 150

in vec4 color;

out vec4 glFragColor;

uniform sampler2D image;
uniform int width;
uniform int height;

void main(void)
{
	float S = gl_FragCoord.s/width;
	float T = gl_FragCoord.t/height;
	float d = texture( image, vec2(S,T)).x;
	if(abs(1-d - gl_FragCoord.z) > 0.000000001)
		discard;
	glFragColor = color;
}
