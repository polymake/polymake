//author Benjamin Kutschan
//default polygon vertex shader
#version 330

uniform mat4 projection;
uniform mat4 modelview;

in vec4 vertex_coordinates;

//!!!!!!!!  if some variable is not initialized properly, don't forget to exclude it
//!!!!!!!!  from the automatic handling by JOGLGeometryInstance.updateAppearance()

void main(void)
{
	gl_Position = projection * modelview * vertex_coordinates;
}
