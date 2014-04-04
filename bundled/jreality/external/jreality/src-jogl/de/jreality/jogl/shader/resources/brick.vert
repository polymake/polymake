//
// Vertex shader for procedural bricks
//
// Authors: Dave Baldwin, Steve Koren, Randi Rost
//          based on a shader by Darwyn Peachey
//
// Copyright (c) 2002-2004 3Dlabs Inc. Ltd. 
//
// See 3Dlabs-License.txt for license information
//

//uniform float rtable[100];
uniform vec3 LightPosition;
uniform float SpecularContribution;
uniform float DiffuseContribution;

varying float LightIntensity;
varying vec2  MCposition;

void main(void)
{
    vec3 ecPosition = vec3 (gl_ModelViewMatrix * gl_Vertex);
    vec3 tnorm      = normalize(gl_NormalMatrix * gl_Normal);
    vec3 lightVec   = normalize(LightPosition - ecPosition);
    vec3 reflectVec = reflect(-lightVec, tnorm);
    vec3 viewVec    = normalize(-ecPosition);
    float diffuse   = max(dot(lightVec, tnorm), 0.0);
    float spec      = 0.0;
    float maxx	= 99.0;
    //int index = (int) clamp(abs(gl_Vertex.x * maxx),0.0,maxx);
    float scale = 1.0 ; //+ .4 * rtable[index];
    vec4 scaler = vec4(scale, scale, scale, 1.0);
    vec4 jitter = scaler * gl_Vertex;

    if (diffuse > 0.0)
    {
        spec = max(dot(reflectVec, viewVec), 0.0);
        spec = pow(spec, 16.0);
    }

    LightIntensity  = DiffuseContribution * diffuse +
                      SpecularContribution * spec;

    
    MCposition      = gl_Vertex.xy;
    //gl_Vertex = gl_Vertex * scale;
    gl_Position     = ftransform();
    //gl_Position = gl_ModelViewProjectionMatrix * jitter;
}
