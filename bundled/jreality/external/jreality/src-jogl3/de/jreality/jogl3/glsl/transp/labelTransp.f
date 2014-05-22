//author Benjamin Kutschan

#version 330

uniform sampler2D tex;

in vec2 tex_Coord2;
out vec4 glFragColor;

//depth checking for transparency
uniform sampler2D _depth;
uniform int _width;
uniform int _height;

void main(void)
{
	float S = gl_FragCoord.s/_width;
	float T = gl_FragCoord.t/_height;
	float d = texture(_depth, vec2(S,T)).x;
	if(abs(1-d - gl_FragCoord.z) > 0.00000001)
		discard;



  glFragColor = texture( tex, tex_Coord2);
  //if(glFragColor.a == 0)
  //	discard;
  //glFragColor = vec4(1.0,0.0,0.0,1.0);
}