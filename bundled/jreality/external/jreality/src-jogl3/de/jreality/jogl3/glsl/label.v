//author Benjamin Kutschan

#version 330

in vec4 vertices;
in vec4 centers;
in vec4 ltwh;

uniform vec4 xyAlignmentTotalWH;
uniform vec4 xyzOffsetScale;

uniform mat4 modelview;
uniform mat4 projection;

//in  vec3 tex_Coord;
out vec2 tex_Coord2;

void main(void)
{
	float scale = xyzOffsetScale.w;
	float xalign = xyAlignmentTotalWH.x;
	float yalign = xyAlignmentTotalWH.y;
	gl_Position = vec4(ltwh.z*scale*xyAlignmentTotalWH.z*(vertices.x+xalign), -ltwh.w*scale*xyAlignmentTotalWH.w*(vertices.y+yalign), ltwh.z*scale*xyAlignmentTotalWH.z*vertices.z, 1);
	gl_Position = gl_Position + modelview*centers + vec4(0,0,0,-1) + vec4(xyzOffsetScale.xyz,0);
	
	gl_Position = projection*gl_Position;
	
	tex_Coord2 = vec2(ltwh.x+ltwh.z*vertices.x, ltwh.y+ltwh.w*vertices.y);
}