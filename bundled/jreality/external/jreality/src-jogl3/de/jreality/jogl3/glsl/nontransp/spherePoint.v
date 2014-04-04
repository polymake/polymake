//author Benjamin Kutschan
//default point shader, no spheres draw, vertex
#version 330

uniform mat4 projection;
uniform mat4 modelview;

in vec4 vertex_coordinates;
uniform int has_vertex_relativeRadii;
in float vertex_relativeRadii;
uniform int has_vertex_colors;
in vec4 vertex_colors;
in vec4 sphere_coords;

uniform float pointShader_pointRadius;
uniform int pointShader_radiiWorldCoordinates;

out vec4 camSpaceCoord;
out vec3 camSpaceNormal;
out vec4 color;

void main(void)
{
	color = vec4(1, .5, .5, 1);
	if(has_vertex_colors == 1)
		color = vertex_colors;
	vec4 vertex = vertex_coordinates;
	float radius = 1;
	if(has_vertex_relativeRadii == 1)
		radius = vertex_relativeRadii;
	if(pointShader_radiiWorldCoordinates == 1){
		camSpaceCoord = modelview * vertex - pointShader_pointRadius * radius * vec4(sphere_coords.xyz, 0);
		//mat3 rotation = mat3(vec3(modelview[0][0], modelview[0][1], modelview[0][2]), vec3(modelview[1][0], modelview[1][1], modelview[1][2]), vec3(modelview[2][0], modelview[2][1], modelview[2][2]));
		camSpaceNormal = sphere_coords.xyz; //normalize(rotation*sphere_coords.xyz);
	}else{
		camSpaceCoord = modelview * (vertex - pointShader_pointRadius * radius * vec4(sphere_coords.xyz, 0));
		mat3 rotation = mat3(vec3(modelview[0][0], modelview[0][1], modelview[0][2]), vec3(modelview[1][0], modelview[1][1], modelview[1][2]), vec3(modelview[2][0], modelview[2][1], modelview[2][2]));
		camSpaceNormal = normalize(rotation*sphere_coords.xyz);
	}
	
	gl_Position = projection * camSpaceCoord;
}