//author Benjamin Kutschan
//default polygon vertex shader
#version 330

uniform mat4 projection;
uniform mat4 modelview;

uniform int polygonShader_smoothShading = 1;

//shadow map samplers later

//VERTEX ATTRIBUTES
in vec4 vertex_coordinates;

uniform int has_vertex_normals;
in vec3 vertex_normals;

uniform int has_face_normals;
in vec3 face_normals;

//!!!!!!!!  if some variable is not initialized properly, don't forget to exclude it
//!!!!!!!!  from the automatic handling by JOGLGeometryInstance.updateAppearance()

uniform vec3 polygonShader_LightPosition;
uniform float polygonShader_SpecularContribution;
uniform float polygonShader_DiffuseContribution;

out float LightIntensity;
out vec2  MCposition;

void main(void)
{
	vec3 ecPosition = vec3 (modelview * vertex_coordinates);
	
	vec3 normals = vec3(0.57735, 0.57735, 0.57735);
	if(polygonShader_smoothShading==0 && has_face_normals==1)
		normals = face_normals;
	else if(polygonShader_smoothShading==1 && has_vertex_normals==1)
		normals = vertex_normals;
	else if(has_vertex_normals==1)
		normals = vertex_normals;
	else if(has_face_normals == 1)
		normals = face_normals;
	
	mat3 rotation = mat3(vec3(modelview[0][0], modelview[0][1], modelview[0][2]), vec3(modelview[1][0], modelview[1][1], modelview[1][2]), vec3(modelview[2][0], modelview[2][1], modelview[2][2]));
	vec3 tnorm = normalize(rotation*normals);
	
    vec3 lightVec   = normalize(polygonShader_LightPosition - ecPosition);
    vec3 reflectVec = reflect(-lightVec, tnorm);
    vec3 viewVec    = normalize(-ecPosition);
    float diffuse   = max(dot(lightVec, tnorm), 0.0);
    float spec      = 0.0;
    float maxx	= 99.0;
    //int index = (int) clamp(abs(gl_Vertex.x * maxx),0.0,maxx);
    float scale = 1.0 ; //+ .4 * rtable[index];
    vec4 scaler = vec4(scale, scale, scale, 1.0);
    vec4 jitter = scaler * vertex_coordinates;

    if (diffuse > 0.0)
    {
        spec = max(dot(reflectVec, viewVec), 0.0);
        spec = pow(spec, 16.0);
    }

    LightIntensity  = polygonShader_DiffuseContribution * diffuse +
                      polygonShader_SpecularContribution * spec;

    
    MCposition      = vertex_coordinates.xy;
    
    gl_Position = projection * modelview * vertex_coordinates;
}
