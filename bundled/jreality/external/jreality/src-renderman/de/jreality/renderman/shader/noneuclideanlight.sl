/* noneuclideanlight.sl:	shader for point lights in hyperbolic space
 * Author:	Charlie Gunn
 * Description: 
 *	See noneuclideancpolygonshader.sl also. 
 *	This simulates a point light in noneuclidean space.  In the hyperbolic case,
 *  the brightness of the light
 *	decays exponentially since the surface area of the sphere of radius r in hyperbolic
 *	space is exp(kr) for some constant k.    Distance is given by arccosh({p1,p0}),
 *	where {p1,p0} is the Minkowski inner product on projective space.
 *  Similar considerations apply to the spherical/elliptic case.
 */

#include "P3.h"

light
noneuclideanlight(
	float atten1 = .5,         
        atten2 = 2.0,
	intensity = 1;
	color lightcolor = 1;
    point from = point "current" (0,0,0);	/* light position */
    float signature = -1.0;
    point to = (0,0,1);
	)
{
	Cl = 0;
    if (signature == 0) {
        illuminate( from )
	        Cl = intensity * lightcolor / L.L;
    } else {
        uniform float debug = 1.0;
        float broken = 0;
        color brokeC = 0;
        vector fromV = from;
        vector Pv = Ps;
	    float PP =  dot(Pv, Pv, signature); //1.0 + signature* sP.sP;
        if (PP > 0.0)   { //debug != 0.0)
            if (debug != 0.0) {
                brokeC = color(1,0,0);
                printf("point has norm squared %f\n",PP);
            }
            broken = 1;
        } 

	    illuminate(from)	{
		    float QQ =  dot(fromV, fromV, signature); //1.0 + signature * fromV.fromV;
            //if (debug != 0.0) printf("Light has norm squared %f\n",PP);
		    if (QQ > 0 ) {
                if (debug != 0.0) {
                    brokeC = color(0,1,0);
                    printf("point has norm squared %f\n",QQ);
                } 
                broken = 1;
                QQ = 0;
            }
		    float PQ =  abs(dot(Pv, fromV, signature)); //1.0 + signature * sP.fromV;
		    //if (PQ <= 0 ) PQ = 0; 
	    	
		    float d = PP * QQ;
            // not sure what it means for the following not to be true
		    //if (d > 0) {
		        float inpro = PQ / sqrt (d);
                float t;
                if (signature == -1) {
		            float hdis = acosh(inpro); 
                    t = atten2*exp(-atten1*hdis);
                }
                else {
                    float hdis = acos(inpro);
                    t = atten2 * abs(sin(hdis)); 
                }
		        Cl += intensity * lightcolor * t;
		    //} else
                //Cl += intensity * lightcolor; 
		}
        if (broken != 0.0) {
            Cl = brokeC;
            //printf("Broken color\n");
        }
        //printf("Color = %c\n", Cl);
    }
}


