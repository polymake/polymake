//author Benjamin Kutschan

#version 330

uniform sampler2D tex;

in vec2 tex_Coord2;
out vec4 glFragColor;

void main(void)
{
  glFragColor = texture( tex, tex_Coord2);
  if(glFragColor.a == 0)
  	discard;
  //glFragColor = vec4(1.0,0.0,0.0,1.0);
}