//author Benjamin Kutschan
//default point shader, no spheres draw, vertex
#version 330

uniform mat4 projection;
uniform mat4 modelview;
//uniform vec4 camPosition;

in vec4 vertex_coordinates;
in float vertex_relativeRadii;
in vec4 vertex_colors;

uniform int pointShader_attenuatePointSize;
uniform float pointShader_pointRadius;
uniform float pointShader_pointSize;
uniform float screenSize;
uniform float screenSizeInSceneOverScreenSize;

out float x;
out float y;
out float z;
out float w;
out float screenPortion;

out vec4 color;
void main(void)
{
	color = vertex_colors;

	vec4 posInCamSpace = modelview * vertex_coordinates;
	gl_Position = projection * posInCamSpace;
	
	
	float dist = length(posInCamSpace.xyz);
	//float magn = screenSize/100.0;
	//float pointSize2 = - (screenSize/100.0)*pointSize/posInCamSpace.z;
	float pointSize2 = - 4*pointShader_pointRadius/(posInCamSpace.z*screenSizeInSceneOverScreenSize);
	screenPortion = screenSizeInSceneOverScreenSize*pointSize2;
	gl_PointSize = pointSize2*vertex_relativeRadii;
	if(pointShader_attenuatePointSize == 0){
		gl_PointSize = vertex_relativeRadii * pointShader_pointSize/2;
		gl_PointSize = pointShader_pointSize*2;
	}
	z = posInCamSpace.z;
	w = posInCamSpace.w;
	x = posInCamSpace.x;
	y = posInCamSpace.y;
}