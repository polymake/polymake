#include "R4.h"

float 
dot(float v1[4], v2[4], signature; ) {
    return v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2] + signature *  v1[3]*v2[3];
}

float 
dot(vector v1, v2; float signature; ) {
    return v1.v2 + signature;
}

float 
dot(point v1, v2; float signature; ) {
    return v1.v2 + signature;
}

float
acosh(float inpro;) {
    return log(inpro + sqrt((abs(inpro*inpro - 1))));
}

void 
hnormalize(output float v[4]; float signature;)   {
float t = dot(v,v, signature);
t = 1.0/sqrt(abs(t));
    //if (v[3] < 0) t = -t;
    times(v,t,v);
}

void hmaketangentNormalized(float p[4]; output float v[4]; float signature; ) {
    float t = -signature*dot(v, p, signature);
    linearCombination(v,1,v,t,p);
}

void 
hmaketangent(float p[4]; output float v[4]; float signature; ) {
    //extern float Pa[4];
    //extern uniform float signature;
    float t = -dot(v, p, signature);
    float s = dot(p,p, signature);
    linearCombination(v,s,v,t,p);
}

