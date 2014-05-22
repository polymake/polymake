//author Benjamin Kutschan
//default polygon fragment shader
#version 330

uniform sampler2D image;
uniform int width;
uniform int height;

void main(void)
{
	float S = gl_FragCoord.s/width;
	float T = gl_FragCoord.t/height;
	float d = texture( image, vec2(S,T)).x;
	
	if(d - gl_FragCoord.z < 0.000000001)
		discard;
	
	gl_FragDepth = 1-gl_FragCoord.z;
}