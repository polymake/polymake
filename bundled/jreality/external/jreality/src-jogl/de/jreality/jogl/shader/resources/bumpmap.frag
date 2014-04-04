varying vec3 lightVec;
varying vec3 eyeVec;
varying vec2 texCoord;
uniform sampler2D colorMap;
uniform sampler2D normalMap;
uniform samplerCube envMap;
uniform float invRadius;

const float texScale=30;

void main (void)
{
	float distSqr = dot(lightVec, lightVec);
	float att = clamp(1.0 - invRadius * sqrt(distSqr), 0.0, 1.0);
	vec3 lVec = lightVec * inversesqrt(distSqr);

	vec3 vVec = normalize(eyeVec);
	
	vec4 base = texture2D(colorMap, texScale*texCoord);
	float alpha = base.w;
	
	vec3 bump = normalize( texture2D(normalMap, texScale*texCoord).xyz * 2.0 - 1.0);
	if (dot(vVec, bump) < 0) bump *= -1;


	vec4 vAmbient = /*gl_LightSource[0].ambient * */gl_FrontMaterial.ambient;

	float diffuse = max( dot(lVec, bump), 0.0 );
	
	vec4 vDiffuse = /*gl_LightSource[0].diffuse * */gl_FrontMaterial.diffuse * 
					diffuse;	

	float specular = pow(clamp(dot(reflect(-lVec, bump), vVec), 0.0, 1.0), 
	                 gl_FrontMaterial.shininess );
	
	vec4 vSpecular = /*gl_LightSource[0].specular * */gl_FrontMaterial.specular * specular;	
	
	gl_FragColor = (vAmbient*base + vDiffuse*base + vSpecular) * att;
	gl_FragColor.w = alpha;
}
