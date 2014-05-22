/**
 *
 * This file is part of jReality. jReality is open source software, made
 * available under a BSD license:
 *
 * Copyright (c) 2003-2006, jReality Group: Charles Gunn, Tim Hoffmann, Markus
 * Schmies, Steffen Weissmann.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of jReality nor the names of its contributors nor the
 *   names of their associated organizations may be used to endorse or promote
 *   products derived from this software without specific prior written
 *   permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* This is a non-euclidean polygon shader.
 * It does not have all the features of the standard DefaultPolygonShader.
 * Author:  Charles Gunn
 */
vec4 Ambient;
vec4 Diffuse;
vec4 Specular;
vec4 texcoord;
attribute vec4 normals4;
uniform bool hyperbolic;
uniform bool 	useNormals4;
uniform bool 	lightingEnabled; 
uniform bool 	fogEnabled;
uniform bool 	transparencyEnabled;
uniform bool 	twoSided;
uniform float Nw;
uniform float	transparency;
uniform sampler2D texture;
uniform int numLights;
uniform int numTextures;
uniform bool poincareModel;
uniform mat4 cam2H;
uniform mat4 H2NDC;

// the inner product in klein model of hyperbolic space
float dot4(in vec4 P, in vec4 Q)	{
	return P.x*Q.x+P.y*Q.y+P.z*Q.z + (hyperbolic ? -1.0 : 1.0)* P.w*Q.w;
}

// the derived length function
float length4(in vec4 P)	{
    return sqrt(abs(dot4(P,P)));
}

float acosh(in float x) {
    return log(abs(x) + sqrt(abs(x*x-1.0)));
}

float distance4(in vec4 a, in vec4 b)    {
    float d = dot4(a,b)/sqrt(abs(dot4(a,a)*dot4(b,b)));
    if (hyperbolic) 
    		return acosh(-d);
    else return abs(acos(d));
}

// project the vector T into the hyperbolic tangent space of P
void projectToTangent(in vec4 P, inout vec4 T) {
		T = (dot4(P,P) * T - dot4(P,T) * P);
}

// find the representative of the given point with length +/- 1
void normalize4(inout vec4 P)	{
    P = (1.0/length4(P))*P;
}
 
// adjust T to be a unit tangent vector to the point P
void normalize4(in vec4 P, inout vec4 T)	{
	projectToTangent(P,T);
	normalize4(T);
}

void dehomogenize(inout vec4 P4)	 {
    float w = P4.w;
    if (w == 0.0) return;
    w = 1.0/w;
    P4 = w * P4;
}

// calculate the lighting incident on a position with given normal vector and 
// given eye position (lights are available as global array).
void pointLight(in int i, in vec4 normal, in vec4 eye, in vec4 ecPosition4)
{
   float nDotVP;       // normal . light direction
   float nDotHV;       // normal . light half vector
   float pf;           // power factor
   float attenuation;  // computed attenuation factor
   float d;            // distance from surface to light source
   vec4  toLight;           // direction from surface to light position
   vec4  halfVector;   // direction of maximum highlights

   // Compute distance between surface and light position
   d = distance4(gl_LightSource[i].position, ecPosition4);
   toLight = gl_LightSource[i].position;
    // Normalize the vector from surface to light position
   normalize4(ecPosition4, toLight );

 //   Compute attenuation
   if (hyperbolic) 
   	  attenuation = gl_LightSource[i].constantAttenuation * exp(-gl_LightSource[i].linearAttenuation * d);
   else attenuation =  gl_LightSource[i].constantAttenuation+(1.0-gl_LightSource[i].linearAttenuation)*abs(cos(d));

    halfVector = (toLight + eye);
    if (hyperbolic) halfVector = -halfVector;
   normalize4(ecPosition4, halfVector); 
   nDotVP = max(0.0, dot4(normal, toLight));
   nDotHV = max(0.0, dot4(normal, halfVector));

   if (nDotVP == 0.0) pf = 0.0;
   else 
		pf = pow(nDotHV, gl_FrontMaterial.shininess);

   Ambient  += gl_LightSource[i].ambient * attenuation;
   Diffuse  += gl_LightSource[i].diffuse * nDotVP * attenuation;
   Specular += gl_LightSource[i].specular * pf * attenuation;
}

//void ftexgen(in vec4 normal, in vec4 ecPosition)
//{
//    gl_TexCoord[0] = gl_TextureMatrix[0]*gl_MultiTexCoord0;
//}

vec4 light(in vec4 normal, in vec4 ecPosition, in gl_MaterialParameters matpar)
{
    vec4 color;
    vec4 eye = vec4(0.0, 0.0, 0.0, 1.0);
    int i;
    float fog = 0.0, d2eye=0.0, alpha;
    if (fogEnabled)	{
      	d2eye = distance4(eye, ecPosition); 
//    d2eye = 1.0;
    	fog = exp2(-d2eye * gl_Fog.density);
    	fog = clamp(fog, 0.0, 1.0);
    }   
    // Clear the light intensity accumulators
    if (lightingEnabled)	{
        normalize4(ecPosition, eye);
        Ambient  = Diffuse = Specular = vec4 (0.0);
        for ( i = 0; i<numLights; ++i)	{
    	    pointLight(i, normal, eye, ecPosition);
        }
   		color = gl_FrontLightModelProduct.sceneColor +
      	    Ambient  * matpar.ambient +
      	    Diffuse  * gl_Color + // matpar.diffuse  +
      	    Specular * matpar.specular;
    } else  {
   		color = matpar.diffuse; //gl_FrontLightModelProduct.sceneColor +
      	   //matpar.ambient +
      	   //matpar.diffuse;
    }

    color = clamp( color, 0.0, 1.0 );
    if (fogEnabled) color = mix( (gl_Fog.color), color, fog);
    color.a = 1.0-transparency;
    if (color.a != 0.0 && !transparencyEnabled) color.a = 1.0;
   return color;
}

void main (void)
{
	bool normals4d = false;
	// various ugly hacks used here to ship over the normals
	if  (gl_Fog.start > 0.0) normals4d = true;
	vec4 n4 = (normals4d) ? gl_MultiTexCoord3 : vec4(gl_Normal, Nw);
    vec4  transformedNormal = gl_ModelViewMatrix * n4; 
    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex ;
    normalize4(ecPosition);
    normalize4(ecPosition, transformedNormal);
    if (hyperbolic && transformedNormal.w * transformedNormal.z > 0.0) 
    	transformedNormal = -transformedNormal;
// set the texture coordinate
    gl_TexCoord[0] = texcoord = gl_TextureMatrix[0]*gl_MultiTexCoord0;
    gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;
    gl_FrontColor = light(transformedNormal, ecPosition, gl_FrontMaterial);
    transformedNormal = -transformedNormal;
    gl_BackColor = light(transformedNormal, ecPosition, gl_BackMaterial);
    if (gl_BackColor.r + gl_BackColor.g + gl_BackColor.b < .01) gl_BackColor = gl_FrontColor;
    else if (gl_FrontColor.r + gl_FrontColor.g + gl_FrontColor.b < .01) gl_FrontColor = gl_BackColor;
    if (poincareModel)	{
        // p4 is in the coordinate system of H3
      	vec4 p4 =  cam2H * ecPosition;
     	dehomogenize(p4);
     	float d = length4(p4);
     	float s = 1.0/(1.0+d);
    	p4.x = s * p4.x;
     	p4.y = s * p4.y;
     	p4.z = s * p4.z;
    	gl_Position = H2NDC * p4; 
//     	gl_Position = gl_ModelViewProjectionMatrix * ( gl_ModelViewMatrixInverse * (H2NDC * p4)); 
     }
	else     gl_Position = ftransform();
}

