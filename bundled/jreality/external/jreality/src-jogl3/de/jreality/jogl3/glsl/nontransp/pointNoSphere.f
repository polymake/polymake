//author Benjamin Kutschan
//default point shader, no spheres draw, fragment
#version 330

uniform mat4 projection;

uniform sampler2D sys_tex;

out vec4 glFragColor;
uniform vec4 pointShader_diffuseColor;
uniform int lightingEnabled;

in float x;
in float y;
in float z;
in float w;
in float screenPortion;

vec3 light = vec3(0.57735, 0.57735, 0.57735);
in vec4 color;
uniform int has_vertex_colors;
void main(void)
{
	//pixel color from texture
	vec4 tex = texture(sys_tex, gl_PointCoord);
	
	//depth calculations:
	//transform coordinates to the unit circle
	vec2 coordsxy = 2*vec2(gl_PointCoord.x-.5, gl_PointCoord.y-.5);
	//calculate the distance from the origin
	float radius = length(coordsxy);
	//z coordinate in the unit sphere
	float coordsz = sqrt(1-radius*radius);
	float shade2 = dot(vec3(coordsxy, coordsz), light);
	if(radius >= 1){
		discard;
	}else{
		//float offset = sqrt(1-radius*radius);
		float omega = 0.5*coordsz*screenPortion*z;
		vec4 windowCoords = projection * vec4(x, y, z-omega, w);
		gl_FragDepth = 0.5+0.5*windowCoords.z/windowCoords.w;
	}
	
	vec4 color2 = pointShader_diffuseColor;
	if(has_vertex_colors == 1)
		color2 = color;
	
	//either this texture lookup method
	float shade;
	if(lightingEnabled == 1)
		shade = tex.b;
	else
		shade = .5;
	if(shade < .5){
		float red = color2.r*2*shade;
		float green = color2.g*2*shade;
		float blue = color2.b*2*shade;
		glFragColor = vec4(red, green, blue, tex.a);
	}else{
		shade = (shade-.5)*2;
		float red = color2.r + shade - color2.r*shade;
		float green = color2.g + shade - color2.g*shade;
		float blue = color2.b + shade - color2.b*shade;
		glFragColor = vec4(red, green, blue, tex.a);
	}
	
	//or using the normal directly
	//glFragColor = vec4((shade2*diffuseColor).rgb, 1);
	//gl_FragDepth = .99;//gl_FragCoord.z;
	//glFragColor = vec4(1, 0, 0, 1);
}