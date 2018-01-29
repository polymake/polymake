
#version 3.1;
#global_settings { assumed_gamma 1.0 }
#global_settings { max_trace_level 20.0 }

//macro for drawing arrows
#macro arrow (fromNode, toNode, edgeThickness, arrowHeadLength, arrowHeadWidth)
  #local edgeLength = vlength(toNode-fromNode);
  #local arrowHeadLength = min(arrowHeadLength,0.5*edgeLength);
  #local headBase = fromNode + (toNode-fromNode)*(edgeLength-arrowHeadLength)/edgeLength;
  union {
    cylinder { fromNode, headBase, edgeThickness }
    cone     { headBase, arrowHeadWidth*edgeThickness, toNode, 0}
  }
#end

//macro for drawing edges
#macro capsule(fromNode, toNode, edgeThickness)
   union {
      cylinder { fromNode, toNode, edgeThickness }
      sphere   { fromNode, edgeThickness }
      sphere   { toNode  , edgeThickness }
   }
#end	

#include "colors.inc"
#include "metals.inc"
#include "finish.inc"

#declare origin = <0,0,0>;
#declare thick = 0.03;

// due to Wolfram Schlickenrieder
#declare T_Polytope_solid = 
texture {
	pigment { color rgbf <1,1,1, 1> }
	finish {
		specular 0.3
		roughness 0.001
		ambient 0.1
		diffuse 0.2
		reflection {
			0.2, 1.0
			fresnel on
		}
		conserve_energy
	}
}

#declare T_Polytope_wires =
  texture {
    pigment { color rgb <0.39, 0.41, 0.6> }
    finish { F_MetalC }
  }

#declare T_Polytope_nodes =
  texture {
    pigment { color rgbf <1, 0, 0, 1> }
    finish { 
      ambient 0.1
      diffuse 0.6
      specular 0.2
    }
  }


// light near camera
// light_source {
//   <0, 1.6, -2.7>
//   color White
//   spotlight
//   radius 20
//   falloff 25
//   tightness 10
//   area_light <2, 0, 0>, <0, 0, 2>, 5, 5
//   adaptive 1
//   jitter
//   point_at origin
// }

//light from above
light_source {
  <1, 10, -2>
  color White
  spotlight
  radius 100
  falloff 25
  tightness 10
  area_light <1, 0, 0>, <0, 1, 0>, 1, 1
  adaptive 1
  jitter
  point_at origin
}

// An area light (creates soft shadows)
// WARNING: This special light can significantly slow down rendering times!
light_source {
  0*x                 // light's position (translated below)
  color rgb 1.0       // light's color
  area_light
  <8, 0, 0> <0, 0, 8> // lights spread out across this distance (x * z)
  4, 4                // total number of lights in grid (4x*4z = 16 lights)
  adaptive 0          // 0,1,2,3...
  jitter              // adds random softening of light
  circular            // make the shape of the light circular
  orient              // orient light
  translate <40, 80, -40>   // <x y z> position of light
}

camera {
  location 0.7*<1, 2.2, -2.7>
  right    <-1.33,0,0>
  look_at  <0,-.5,0>	
}


plane { y, -3
  pigment { color rgbf<1,1,.5,0> }
  finish {
    reflection .1 specular .1
    ambient 0.6 diffuse 0.2 
  }
  //  normal { ripples 4 turbulence 5  scale 3 }
}
