//author Benjamin Kutschan
//default polygon fragment shader
#version 330


#define Integral(x, p, notp)	((floor(x)*(p)) +  max(fract(x)-(notp), 0.0))

out vec4 glFragColor;

uniform vec3  polygonShader_BrickColor;
uniform vec3 polygonShader_MortarColor;
uniform vec2  polygonShader_BrickSize;
uniform vec2  polygonShader_BrickPct;
uniform vec2 polygonShader_MortarPct;

in vec2  MCposition;
in float LightIntensity;

void main(void)
{
	vec3  color;
    vec2  position, useBrick, fw;
    
    position = MCposition / polygonShader_BrickSize;

    if (fract(position.y * 0.5) > 0.5)
        position.x += 0.5;

    // position = fract(position);
    fw = fwidth(position);

    //if (fw == 0.0) useBrick = step(position, BrickPct);
    //else 
    	useBrick = (Integral(position+fw, polygonShader_BrickPct, polygonShader_MortarPct) - Integral(position, polygonShader_BrickPct, polygonShader_MortarPct))/fw;

    float mixer = useBrick.x * useBrick.y;
    if(mixer==0)
    	discard;
    //if (mixer == 0.0) discard;
    color  = mix(polygonShader_MortarColor, polygonShader_BrickColor, mixer);
    color *= LightIntensity;
    glFragColor = vec4 (color, mixer);
    
}
