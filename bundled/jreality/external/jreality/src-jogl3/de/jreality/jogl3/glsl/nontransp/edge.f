//author Benjamin Kutschan
//default line fragment shader
#version 330

out vec4 glFragColor;
uniform vec4 lineShader_lineDiffuseColor;
uniform vec4 lineShader_diffuseColor;
uniform int lineShader_lineLighting;

in vec4 edgeColor;
in vec4 vertexColor;
uniform int has_vertex_colors;
uniform int lineShader_vertexColors;
uniform int has_edge_colors;

void main(void)
{
	if(lineShader_lineLighting == 1)
		glFragColor = lineShader_diffuseColor;
	else
		glFragColor = lineShader_lineDiffuseColor;
	
	if(has_edge_colors == 1)
		glFragColor = edgeColor;
	if(has_vertex_colors == 1 && lineShader_vertexColors == 1)
		glFragColor = vertexColor;
}