//
// Fragment shader for jReality tutorial
//
// Authors: Charles Gunn

uniform sampler2D  sampler;
uniform sampler2D sampler2;
uniform float BlendFactor;
void main(void)
{
    vec4 currentSample = texture2D(sampler,gl_TexCoord[0].st); 
    vec4 currentSample2 = texture2D(sampler2,gl_TexCoord[1].st);
    float alpha = BlendFactor * currentSample2.a; 
    gl_FragColor.rgb = mix(currentSample.rgb, currentSample2.rgb, alpha); //( currentSample.rgb * (1.0-alpha) + currentSample2.rgb *alpha); 
    gl_FragColor.a = 1.0;
}
