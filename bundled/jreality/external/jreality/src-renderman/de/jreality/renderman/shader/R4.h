void
printArrays(string label; float v[4]; string label2; float v2[4]) {
    float i = 0;
    printf("%s %f %f %f %f\n%s %f %f %f %f\n", 
            label, v[0], v[1], v[2], v[3],
            label2, v2[0], v2[1], v2[2], v2[3]);
}
void
printArray(string label; float v[4]) {
    float i = 0;
    printf("%s %f %f %f %f\n", label, v[0], v[1], v[2], v[3]);
}
void
printArray3(string label; float v[4]) {
    float i = 0;
    printf("%s %f %f %f\n", label, v[0]/v[3], v[1]/v[3], v[2]/v[3]);
}
void
matrixTimesVector(output float dst[4] ; float m[16] ; float v[4])  {
   dst[0] = m[0]*v[0]+m[4]*v[1]+m[8]*v[2]+m[12]*v[3];
   dst[1] = m[1]*v[0]+m[5]*v[1]+m[9]*v[2]+m[13]*v[3];
   dst[2] = m[2]*v[0]+m[6]*v[1]+m[10]*v[2]+m[14]*v[3];
   dst[3] = m[3]*v[0]+m[7]*v[1]+m[11]*v[2]+m[15]*v[3];
}

void
linearCombination(output float a[4]; float s; float v[4]; float t; float w[4])  {
    a[0] = s * v[0] + t * w[0];
    a[1] = s * v[1] + t * w[1];
    a[2] = s * v[2] + t * w[2];
    a[3] = s * v[3] + t * w[3];
}

void
times(output float a[4];  float t; float b[4])  {
    a[0] =  t * b[0];
    a[1] =  t * b[1];
    a[2] =  t * b[2];
    a[3] =  t * b[3];
}

