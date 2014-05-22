/* 
 *  noneuclideanpolygonshader.sl:	a generic surface shader for noneuclidean spaces
 *  Author:		Charles Gunn
 *  Description:
 *	The parameters to this shader are, ideally, the same as the parameters to the regular 
 *	defaultpolygonshader, but this shader computes angles and distances using a  noneuclidean 
 *  metric.
 *   	This metric is defined on homogeneous coordinates (x,y,z,w) and is induced by the 
 *	quadratic form x*x + y*y +z*z + signature* w*w. (signature = 1 or -1)
 *	The hyperbolic metric (signature = -1)  is valid on the interior of the unit ball; 
 *  the isometries of this metric
 *	are projective transformations that preserve the unit sphere and map the interior to 
 *	itself.  These are represented by 4x4 matrices hence can be implemented using the 
 *	regular geometry/viewing pipeline provided by Renderman (and other rendering systems).
 */

#include "P3.h"
surface
noneuclideanpolygonshader ( 
    varying float Nw[4] = {0,0,0,23};
    float Ka = 0, 
	    Kd = .5, 
	    Ks = .5, 
	    roughness = .001, 
	reflectionBlend = .6;
    uniform float objectToCamera[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    uniform float tm[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
	color specularcolor = 1;
	string texturename = ""; 
	string reflectionmap = ""; 
    float lighting = 1,
        transparencyenabled = 1,
        signature = -1;         // shader should also work for elliptic case (+1)
	)
{
	color	total;
    color Ct, Cr;
    float tr = 1;
    float Pa[4] = {xcomp(P), ycomp(P), zcomp(P), 1};
    float debug = 1;


    float PP = dot(Pa,Pa, signature);
	/* handle case that hyperbolic point lies outside unit ball */
	if (signature == -1 && PP > 0)  {
		if (debug != 0) Ci = color(1,0,0);      // draw it in red: debug mode probably 
		Oi = 1.0;
    }

	else {
        if (texturename != ""){
            matrix textureMatrix = matrix "current" (tm[0],tm[1],tm[2],tm[3],tm[4],tm[5],tm[6],tm[7],tm[8],tm[9],tm[10],tm[11],tm[12],tm[13],tm[14],tm[15]);
	        point a = point (s,t,0);
	        point p = transform( textureMatrix , a);
	        float ss = xcomp(p);
	        float tt = ycomp(p);
	        tr = float texture(texturename[3],ss,tt, "fill",1);
            Ct = color texture (texturename,ss, tt);
            Ct = Cs * Ct; 
        }
        else Ct = Cs;
        Oi = Os*tr;

        // normalize the point
        hnormalize(Pa, signature);
        // calculate the normal
        float tN[4];
        point N3;
        // handle the case that the 4D normal has not been sent over
        if (Nw[3] == 23)    {   
            // create a valid normal vector from the geometric normal
	        point Nh = faceforward(normalize(N), I);
            tN[0] = xcomp(Nh); tN[1] = ycomp(Nh); tN[2] = zcomp(Nh); tN[3] = -signature*P.Nh;
            hnormalize(tN, signature);
            }
        else {
            // the host should send over normals in camera coordinates!
            matrixTimesVector(tN, objectToCamera, Nw);
        }
        // calculate the vector from surface to eye
	    // as a difference vector, I has w cord = 0 
        float Ia[4] = {-xcomp(I), -ycomp(I), -zcomp(I), 0};
        hmaketangentNormalized(Pa, Ia, signature);
        hnormalize(Ia, signature);

        float ndi = dot(Ia,tN, signature);
        // flip the normal to have acute angle with eye vector
        if (ndi < 0) times(tN, -1, tN); 

        // and add in the reflection map (like a specular highlight, without modulating by shading)
        // TODO implement non-euclidean reflection
        //if (reflectionmap != "") {
            //D = reflect(I, Nf) ;
            //D = vtransform ("world", D);
            //Ct = (1-reflectionBlend) * Ct + reflectionBlend * color environment(reflectionmap, D);
        //} 

        total = 0;
	    uniform float spec = 1.0/roughness;
       
        illuminance(P)
        {
	        // first adjust light vector to be tangent at P 
	        // L is a vector, hence its w-cord = 0 
            float La[4] = {xcomp(L), ycomp(L), zcomp(L), 0};
            hmaketangentNormalized(Pa, La, signature);
            hnormalize(La, signature);
    
	        float diffuse = dot(La, tN, signature);
            // flip the direction for diffuse illumination
            if (diffuse < 0) diffuse  =  -diffuse;

	        // now compute bisector of angle between L and I 
	        // important for La, Ia to be unit length 
            float M[4];
            linearCombination(M, 1, Ia, 1, La);
            // we don't actually have to make M a tangent vector since it's the sum
            // of two valid tangent vectors
            //hmaketangentNormalized(Pa, M, signature);
            hnormalize(M, signature);
	        // compute cos(angle between normal and mid-vector M) 
	        float ss = dot(M,tN, signature);
	        if (ss < 0.0)	ss =  0;
            
	        total = total + Os * Cl * (Ct *  (Ka*ambient() + Kd*diffuse)+
	                specularcolor * Ks*pow(ss,spec)); 
        }
	    Ci = total;
    }
	Oi = Os; 
}		
