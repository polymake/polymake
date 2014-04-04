//uniform bool LocalViewer ;
//uniform bool SeparateSpecular ;
//uniform float specularExponent;
//uniform vec4 diffuseColor;
//uniform vec4 ambientColor;
//uniform vec4 specularColor;
//uniform float ambientCoefficient;
//uniform float diffuseCoefficient;
//uniform float specularCoefficient;
//uniform bool    hyperbolic;
//uniform bool 	useNormals4;
uniform bool 	lightingEnabled; 
uniform bool 	fogEnabled;
uniform bool 	transparencyEnabled;
uniform float	transparency;
uniform int     numLights;
uniform int numTextures;
#pragma optimize(on)

float calculateAttenuation(in float d, in vec3 surfaceToLightVector,  in int i)	{
    float spotDot;
    float spotAttenuation;
	float attenuation = 1.0 / (gl_LightSource[i].constantAttenuation +
										gl_LightSource[i].linearAttenuation*d+
										gl_LightSource[i].quadraticAttenuation*d*d);
					
		if (gl_LightSource[i].spotCutoff != 180.0)	{
			spotDot = dot(-surfaceToLightVector, gl_LightSource[i].spotDirection);
			if (spotDot >= gl_LightSource[i].spotCosCutoff)
				attenuation  *= pow(spotDot, gl_LightSource[i].spotExponent);
		}
	return attenuation;	
}
void Light(in int i,
					in vec3 surfaceToCameraVector,
					in vec3 surfaceCameraCoordinates,
                    in vec3 normal,
                    in float shininess,
                    inout vec4 ambient,
                    inout vec4 diffuse,
                    inout vec4 specular)
{
    float nDotVP;
    float nDotHV;
    float pf;
    float attenuation=1.0;
    float d;
    vec3 surfaceToLightVector;
    vec3 halfVector;
   	if ( gl_LightSource[i].position.w == 0.0)	{
        nDotVP = max(0.0, dot(normal, normalize(vec3 (gl_LightSource[i].position))));
 //       vec3 hv = normalize(vec3 (gl_LightSource[i].position)) + normal;
        nDotHV =max(0.0, dot(normal, vec3( gl_LightSource[i].halfVector)));
     } else {
	    	 // compute vector from surface to light position
        surfaceToLightVector = (vec3 (gl_LightSource[i].position))/gl_LightSource[i].position.w - surfaceCameraCoordinates;
		surfaceToLightVector = normalize(surfaceToLightVector);
		halfVector = normalize(surfaceToLightVector+surfaceToCameraVector);
        nDotVP = max(0.0, (dot(normal, surfaceToLightVector)));
        nDotHV = max(0.0, (dot(normal, halfVector))); 
        d = length(surfaceToLightVector);
        attenuation = calculateAttenuation(d, surfaceToLightVector, i);
  }
   	if (nDotVP == 0.0 )	pf = 0.0;
    else  pf = pow(nDotHV, shininess);
    ambient += gl_LightSource[i].ambient ;
    diffuse += gl_LightSource[i].diffuse * nDotVP * attenuation;
    specular += gl_LightSource[i].specular * pf * attenuation;
}

void doLighting(in vec3 normal, in vec3 surfaceToCameraVector, in vec3 surfaceCameraCoordinates, in bool front, inout vec4 color, inout vec4 sc) {
     bool SeparateSpecular = false;	// false results in funny problems that look like color clamping errors
     vec4 amb = vec4(0.0);
    vec4 diff = vec4(0.0);
    vec4 spec = vec4(0.0);
    gl_MaterialParameters mp;
    gl_LightModelProducts lmp;
    if (front) {
   		mp = gl_FrontMaterial;
    		lmp = gl_FrontLightModelProduct;
    } else {
    		mp = gl_BackMaterial;
    		lmp = gl_BackLightModelProduct;
    }
    
    int i;
    if (!lightingEnabled)	{
    		color = gl_Color;
    }  else {
    	// loop through lights
    	for (i = 0; i<numLights; ++i)    {
 			Light(i, surfaceToCameraVector, surfaceCameraCoordinates, normal, mp.shininess, amb, diff, spec);
   	 	}
 		color = lmp.sceneColor + amb*mp.ambient+ diff *   gl_Color; 
 
 //   	if (SeparateSpecular)
 //           sc  = spec * mp.specular;
 //   	else 
            color += spec * mp.specular;   
    }
 }

void main(void)
{
    vec3 surfaceCameraCoordinates, surfaceToCameraVector;
    bool LocalViewer = true;
   
    gl_Position = ftransform();
    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;
   	surfaceCameraCoordinates = (vec3 (ecPosition))/ecPosition.w;
    if (LocalViewer)		{  //gl_Position.w != 0.0) {
         surfaceToCameraVector = normalize(surfaceCameraCoordinates);
    }
    else  {
         surfaceToCameraVector = vec3 (0.0, 0.0, 1.0);
    }
    vec3 normal = gl_NormalMatrix * gl_Normal;
    normal = normalize(normal);
 
//    if (numTextures > 0) {
    	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
//    	if (numTextures > 1) {
    		gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
 //   		if (numTextures > 2) {
  				gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;
//        	}
//        }
//    }
    gl_FogFragCoord = ecPosition.z;
//     if (reflectionTextureUnit != -1)	{
//     	gl_TexCoord[reflectionTextureUnit] = vec4(reflect(normalize(surfaceCameraCoordinates), normal),1.0);
//     }
     
    doLighting(normal, surfaceToCameraVector, surfaceCameraCoordinates, true, gl_FrontColor, gl_FrontSecondaryColor);
    normal = -normal;
    doLighting(normal, surfaceToCameraVector, surfaceCameraCoordinates, false, gl_BackColor, gl_BackSecondaryColor);
    if ((-surfaceToCameraVector.z) < .7170)  {
        gl_FrontColor = vec4(0.0, 0.0, 0.0, 1.0); //vec4(surfaceToCameraVector.x, surfaceToCameraVector.y, 0.0, 1.0);
    }

}
