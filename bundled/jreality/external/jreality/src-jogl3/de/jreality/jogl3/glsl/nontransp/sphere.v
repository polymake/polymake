//author Benjamin Kutschan
//default polygon vertex shader
#version 330

uniform mat4 projection;
uniform mat4 modelview;
//uniform mat4 textureMatrix;

//shadow map samplers later

//VERTEX ATTRIBUTES
in vec4 vertex_coordinates;

out vec4 camSpaceCoord;
out vec3 camSpaceNormal;

//!!!!!!!!  if some variable is not initialized properly, don't forget to exclude it
//!!!!!!!!  from the automatic handling by JOGLGeometryInstance.updateAppearance()

void main(void)
{
	gl_Position = projection * modelview * vertex_coordinates;
	
	mat3 rotation = mat3(vec3(modelview[0][0], modelview[0][1], modelview[0][2]), vec3(modelview[1][0], modelview[1][1], modelview[1][2]), vec3(modelview[2][0], modelview[2][1], modelview[2][2]));
	camSpaceNormal = normalize(rotation*vertex_coordinates.xyz);
	camSpaceCoord = modelview*vertex_coordinates;
}
