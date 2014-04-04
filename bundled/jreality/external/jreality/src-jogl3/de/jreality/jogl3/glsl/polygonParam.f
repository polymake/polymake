//author Benjamin Kutschan
//default polygon fragment shader
#version 330

out vec4 glFragColor;
//needed?
float shade = .5;

uniform int lightingEnabled;

uniform vec4 polygonShader_diffuseColor;
vec4 diffuse;
uniform float polygonShader_diffuseCoefficient;

uniform vec4 polygonShader_ambientColor;
uniform float polygonShader_ambientCoefficient;

uniform vec4 polygonShader_specularColor;
uniform float polygonShader_specularCoefficient;
uniform float polygonShader_specularExponent;

uniform sampler2D image;
uniform int has_Tex;
uniform int _combineMode;

in vec2 texCoord;
in vec4 camSpaceCoord;
in vec3 camSpaceNormal;

//GLOBAL LIGHTS
uniform sampler2D sys_globalLights;
uniform int sys_numGlobalDirLights;
uniform int sys_numGlobalPointLights;
uniform int sys_numGlobalSpotLights;

//LOCAL LIGHTS
uniform sampler2D sys_localLights;
uniform int sys_numLocalDirLights;
uniform int sys_numLocalPointLights;
uniform int sys_numLocalSpotLights;

uniform int has_vertex_texturecoordinates;

vec3 lightInflux = vec3(0, 0, 0);

uniform int has_face_colors;
uniform int has_vertex_colors;
in vec4 faceVertexColor;

//cubemap
uniform float _reflectionMapAlpha;
uniform int has_reflectionMap;
uniform sampler2D front;
uniform sampler2D back;
uniform sampler2D left;
uniform sampler2D right;
uniform sampler2D up;
uniform sampler2D down;

uniform mat4 _inverseCamRotation;
//uniform int reflectionMap;

float attenuation(vec3 att, float dist){
	return 1/(att.x+att.y*dist+att.z*dist*dist);
}

float spot(float exp, vec4 dir, vec4 relPos, float coneAngle){
	float angle = acos(dot(-normalize(relPos.xyz), dir.xyz));
	return pow(cos(angle), coneAngle);
}

void calculateLightInfluxGeneral(vec3 normal, int numDir, int numPoint, int numSpot, sampler2D lights){	
	
	//size of the light texture
	int lightTexSize = numDir*3+numPoint*3+numSpot*5;
	
	for(int i = 0; i < numDir; i++){
		vec4 dir = texture(lights, vec2((3*i+1+0.5)/lightTexSize, 0));
		
		vec4 col = texture(lights, vec2((3*i+0.5)/lightTexSize, 0));
		float intensity = texture(lights, vec2((3*i+2+0.5)/lightTexSize, 0)).r;
		vec4 ambient = polygonShader_ambientColor*col*intensity;
		lightInflux = lightInflux + polygonShader_ambientCoefficient * ambient.xyz;
		
		float dott = dot(normal, normalize(dir.xyz));
		if(dott > 0){
			vec4 diffuse2 = dott*diffuse*col*intensity;
			
			float spec = dot(normal, normalize(normalize(dir.xyz)-normalize(camSpaceCoord.xyz)));
			
			vec4 specular =polygonShader_specularColor*intensity*pow(spec, polygonShader_specularExponent);
			
			vec4 new = polygonShader_specularCoefficient*specular+polygonShader_diffuseCoefficient*diffuse2;
			lightInflux = lightInflux + new.xyz;
		}
	}
	for(int i = 0; i < numPoint; i++){
		//calculate distance between light and vertex, possible also in HYPERBOLIC geom
		vec4 pos = texture(lights, vec2((3*numDir+3*i+1+0.5)/lightTexSize, 0));
		vec4 col = texture(lights, vec2((3*numDir+3*i+0.5)/lightTexSize, 0));
		vec4 tmp = texture(lights, vec2((3*numDir+3*i+2+0.5)/lightTexSize, 0));
		float intensity = tmp.a;
		vec3 att = tmp.xyz;
		vec4 RelPos = pos - camSpaceCoord;
		float dist = length(RelPos);
		float atten = 1/(att.x+att.y*dist+att.z*dist*dist);
		
		vec4 ambient = polygonShader_ambientColor*col*intensity;
		lightInflux = lightInflux + atten*polygonShader_ambientCoefficient * ambient.xyz;
		
		float dott = dot(normal, normalize(RelPos.xyz));
		if(dott > 0){
			vec4 diffuse2 = dott*diffuse*col*intensity;
			
			float spec = dot(normal, normalize(normalize(RelPos.xyz)-normalize(camSpaceCoord.xyz)));
			
			vec4 specular =polygonShader_specularColor*intensity*pow(spec, polygonShader_specularExponent);
			
			
			vec4 new = polygonShader_specularCoefficient*specular+polygonShader_diffuseCoefficient*diffuse2;
			lightInflux = lightInflux + atten*new.xyz;
		}
	}
	for(int i = 0; i < numSpot; i++){
		vec4 pos = texture(lights, vec2((3*numDir+3*numPoint+5*i+2+0.5)/lightTexSize, 0));
		vec4 coneAngles = texture(lights, vec2((3*numDir+3*numPoint+5*i+3+0.5)/lightTexSize, 0));
		vec4 RelPos = pos - camSpaceCoord;
		vec4 dir = texture(lights, vec2((3*numDir+3*numPoint+5*i+1+0.5)/lightTexSize, 0));
		float angle = acos(dot(-normalize(RelPos.xyz), dir.xyz));
		if(angle < coneAngles.x){
			vec4 col = texture(lights, vec2((3*numDir+3*numPoint+5*i+0.5)/lightTexSize, 0));
			vec3 att = texture(lights, vec2((3*numDir+3*numPoint+5*i+4+0.5)/lightTexSize, 0)).xyz;
			float dist = length(RelPos);
			float atten = 1/(att.x+att.y*dist+att.z*dist*dist);
			float intensity = coneAngles.a;
			vec4 ambient = polygonShader_ambientColor*col*intensity;
			float factor = pow(cos(angle), coneAngles.z);
			lightInflux = lightInflux + factor*atten*polygonShader_ambientCoefficient * ambient.xyz;
			
			//light is on the right side of the face
			float dott = dot(normal, normalize(RelPos.xyz));
			if(dott > 0){
				vec4 diffuse2 = dott*diffuse*col*intensity;
				float spec = dot(normal, normalize(normalize(RelPos.xyz)-normalize(camSpaceCoord.xyz)));
				vec4 specular =polygonShader_specularColor*intensity*pow(spec, polygonShader_specularExponent);
			
				vec4 new = polygonShader_specularCoefficient*specular+polygonShader_diffuseCoefficient*diffuse2;
				lightInflux = lightInflux + factor*atten*new.xyz;
			}
		}
	}
}

void calculateGlobalLightInflux(vec3 normal){
	lightInflux = lightInflux + polygonShader_ambientColor.xyz*polygonShader_ambientCoefficient;
	calculateLightInfluxGeneral(normal, sys_numGlobalDirLights, sys_numGlobalPointLights, sys_numGlobalSpotLights, sys_globalLights);
}
void calculateLocalLightInflux(vec3 normal){
	calculateLightInfluxGeneral(normal, sys_numLocalDirLights, sys_numLocalPointLights, sys_numLocalSpotLights, sys_localLights);
}

void main(void)
{
	vec4 texColor = texture( image, texCoord.st);
	
	diffuse = polygonShader_diffuseColor;
	if(has_face_colors == 1 || has_vertex_colors == 1)
		diffuse = faceVertexColor;
	
	vec3 normal = normalize(camSpaceNormal);
	if(lightingEnabled == 1){
		lightInflux = vec3(0, 0, 0);
		if(gl_FrontFacing){
			calculateGlobalLightInflux(normal);
			calculateLocalLightInflux(normal);
		}else{
			calculateGlobalLightInflux(-normal);
			calculateLocalLightInflux(-normal);
		}
		glFragColor = vec4(lightInflux, diffuse.a);
	}else{
		glFragColor = diffuse;
	}
	
	float Product = dot(normalize(camSpaceCoord.xyz), camSpaceNormal);
	if(has_vertex_texturecoordinates==1 && has_Tex == 1 && Product < -0.5){
		if(_combineMode == 0x2100)//GL_MODULATE
			glFragColor = texColor*vec4(glFragColor.xyz, diffuse.a);
		if(_combineMode == 0x1E01)//GL_REPLACE
			glFragColor = texColor;
		if(_combineMode == 0x8570)//GL_COMBINE
			glFragColor = texColor;
		if(_combineMode == 0x2101){//GL_DECAL
			glFragColor.a = diffuse.a;
			glFragColor.rgb = (1-texColor.a)*glFragColor.xyz + texColor.a*texColor.rgb;
		}
		if(_combineMode == 0x0BE2){//GL_BLEND
			glFragColor.a = diffuse.a*texColor.a;
			glFragColor.rgb = (vec3(1,1,1)-texColor.rgb)*glFragColor.xyz + texColor.rgb;
		}
		if(_combineMode == 0x0104){//GL_ADD
			glFragColor.a = diffuse.a*texColor.a;
			glFragColor.rgb = glFragColor.xyz + texColor.rgb;
		}
	}
	
	if(glFragColor.a == 0)
		discard;
	
	if(has_reflectionMap == 1){
		//do environment reflections
		vec3 A = -normalize(camSpaceCoord.xyz);
		vec3 C = -A + 2*dot(A,normal)*normal;
		
		//do this computation in jogl3.JOGLRenderState
		mat4 iRotation = inverse(_inverseCamRotation);
		mat3 rotation = mat3(vec3(iRotation[0][0], iRotation[0][1], iRotation[0][2]), vec3(iRotation[1][0], iRotation[1][1], iRotation[1][2]), vec3(iRotation[2][0], iRotation[2][1], iRotation[2][2]));
		C = rotation*C;
		
		float x = C.x;
    	float y = C.y;
    	float z = C.z;
    	//we are in the front texture
    	if(z>abs(x) && z>abs(y)){
    		float X = x/z;
    		float Y = -y/z;
    		glFragColor = (1-_reflectionMapAlpha)*glFragColor+_reflectionMapAlpha*texture( back, vec2(X/2+.5,Y/2+.5));
   		}else if(z < -abs(x) && z < -abs(y)){
    		float X = x/z;
    		float Y = y/z;
    		glFragColor = (1-_reflectionMapAlpha)*glFragColor+_reflectionMapAlpha*texture( front, vec2(X/2+.5,Y/2+.5));
    	}else if(abs(y)>abs(x)){
    		//floor
    		if(y<0){
    			float X = -x/y;
    			float Z = z/y;
    			glFragColor = (1-_reflectionMapAlpha)*glFragColor+_reflectionMapAlpha*texture( down, vec2(X/2+.5,Z/2+.5));
    		//top
    		}else{
    			float X = x/y;
    			float Z = -z/y;
    			glFragColor = (1-_reflectionMapAlpha)*glFragColor+_reflectionMapAlpha*texture( up, vec2(X/2+.5,-Z/2+.5));
    		}
    	//left or right texture
    	}else{
    		//right
    		if(x>0){
    			float Y = -y/x;
    			float Z = z/x;
    			glFragColor = (1-_reflectionMapAlpha)*glFragColor+_reflectionMapAlpha*texture( right, vec2(-Z/2+.5,Y/2+.5));
    		//left
    		}else if(x<0){
    			float Y = y/x;
    			float Z = -z/x;
    			glFragColor = (1-_reflectionMapAlpha)*glFragColor+_reflectionMapAlpha*texture( left, vec2(Z/2+.5,Y/2+.5));
    		}
    	}
	}
	
}
