/*	spot light source shader
 *	with additional support for falloff 
 */

light spotlightFalloff ( float intensity = 1;
                  color lightcolor = 1;
                  point from = point "shader" (0,0,0);
                  point to = point "shader" (0,0,1);
                  float coneangle = radians(30);
                  float conedeltaangle = radians(5);
                  float beamdistribution = 2;
		  float a0 = 0;
		  float a1 = 0;
		  float a2 = 1;
		  string shadowmap = ""; ) {
	float unoccluded = 1;
	if (shadowmap != "")
	unoccluded *= 1 - shadow (shadowmap, Ps, "blur", 0.01,
					  "samples", 16,
					  "bias", 0.01);
/*
	unoccluded *= 1 - shadow (shadowmap, Ps, "blur", shadowblur,
					  "samples", shadownsamps,
					  "bias", shadowbias);
*/
	uniform vector axis = normalize(to-from);

	illuminate (from, axis, coneangle) {
		float cosangle = (L . axis) / length(L);
		float atten = pow (cosangle, beamdistribution) / (a2*(L . L) + a1 * length(L) + a0);
		atten *= smoothstep (cos(coneangle), cos(coneangle-conedeltaangle),cosangle);
		Cl = atten * intensity * lightcolor*unoccluded;
	}
}

