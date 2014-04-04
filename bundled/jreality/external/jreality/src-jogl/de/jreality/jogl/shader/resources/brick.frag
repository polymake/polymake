//
// Fragment shader for procedural bricks
//
// Authors: Dave Baldwin, Steve Koren, Randi Rost
//          based on a shader by Darwyn Peachey
//
// Copyright (c) 2002-2004 3Dlabs Inc. Ltd.
//
// See 3Dlabs-License.txt for license information
//  
#define Integral(x, p, notp)	((floor(x)*(p)) +  max(fract(x)-(notp), 0.0))

uniform vec3  BrickColor, MortarColor;
uniform vec2  BrickSize;
uniform vec2  BrickPct, MortarPct;

varying vec2  MCposition;
varying float LightIntensity;

void main(void)
{
    vec3  color;
    vec2  position, useBrick, fw;
    
    position = MCposition / BrickSize;

    if (fract(position.y * 0.5) > 0.5)
        position.x += 0.5;

    // position = fract(position);
    fw = fwidth(position);

    //if (fw == 0.0) useBrick = step(position, BrickPct);
    //else 
    	useBrick = (Integral(position+fw, BrickPct, MortarPct) - Integral(position, BrickPct, MortarPct))/fw;

    float mixer = useBrick.x * useBrick.y;
    //if (mixer == 0.0) discard;
    color  = mix(MortarColor, BrickColor, mixer);
    color *= LightIntensity;
    gl_FragColor = vec4 (color, mixer);
}
