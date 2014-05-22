//author Benjamin Kutschan
//default polygon vertex shader
#version 330

uniform mat4 projection;
uniform mat4 modelview;

uniform vec3 camPosition;
uniform vec4 globalLightColor;

uniform int smoothShading;

//change this to a 1D-Texture
uniform vec4[] pointLightColors;
uniform vec4[] pointLightPositions;
uniform float[] pointLightConeAngles;
//shadow map samplers later
uniform int numDirLights;
uniform vec4[4] directionalLightColors;
uniform vec3[4] directionalLightDirections;

//rename to reproduce the vertex attribute names
in vec4 vertex_coordinates;
//in vec3 vertex_normals;
//in vec3 face_normals;

//out float shade;

//vec3 lightDir = vec3(0, .7, -.7);

void main(void)
{
	
	gl_Position = projection * modelview * vertex_coordinates;
	
}