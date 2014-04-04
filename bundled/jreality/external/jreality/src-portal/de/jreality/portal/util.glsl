mat3 rotMat3(in vec3 axis, in float angle) {
	vec3 u = normalize(axis);
	
	float c = cos(angle);
	float s = sin(angle);
	float v = 1.0 - c;
	
	mat3 m = mat3(
	 u[0] * u[0] * v + c,
	 u[0] * u[1] * v + u[2] * s,
	 u[0] * u[2] * v - u[1] * s, // columns

	 u[1] * u[0] * v - u[2] * s,
	 u[1] * u[1] * v + c,
	 u[1] * u[2] * v + u[0] * s,

	 u[2] * u[0] * v + u[1] * s,
	 u[2] * u[1] * v - u[0] * s,
	 u[2] * u[2] * v + c);
	return m;
}

mat4 makePerspectiveProjectionMatrix(in vec4 viewport, in vec4 c)	{
	float an = abs(near);
	float l = viewport[0] * an;
	float r = viewport[1] * an;
	float b = viewport[2] * an;
	float t = viewport[3] * an;
	
	mat4 m = mat4(
	2.*near/(r-l),
	0.,
	0.,
	0.,
	
	0.,
	2.*near/(t-b),
	0.,
	0.,
	
	(r+l)/(r-l),
	(t+b)/(t-b),
	(far+near)/(near-far),
	-1.0,
	
	0.,
	0.,
	2.*near*far/(near-far),
	0.);
	
	mat4 cc = mat4(
	1.,	0.,	0.,	0.,
	
	0.,	1.,	0.,	0.,
	
	0.,	0.,	1.,	0.,
	
	-c.x, -c.y,	-c.y, 1.);
	
	return m*cc;
}
	
vec4 transformViewport(in vec4 cv, in vec4 c) {
	float fscale = 1./(d+c.z);
	vec4 ret = vec4(fscale*(cv.x*d-c.x), 
					 fscale*(cv.y*d-c.x), 
					 fscale*(cv.z*d-c.y), 
					 fscale*(cv.w*d-c.y));
	if (ret.x>ret.y) {
		float t = ret.y;
		ret.y=ret.x;
		ret.x=t;
	}
	if (ret.z>ret.w) {
		float t = ret.w;
		ret.w=ret.z;
		ret.z=t;
	}
	return ret;
}
	