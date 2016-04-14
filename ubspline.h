#ifndef UBSPLINE_H
#define UBSPLINE_H

typedef union vector3_u // Vector 3 can be reached by value, or by array by using the value a.
{
    struct {
        float x, y, z;
    };
    float a[3];
} vector3;

int q(const vector3* list, float t, int derivative, vector3* result);

float ubSpline(float p0, float p1, float p2, float p3, float t);
float ubSplinePrime(float p0, float p1, float p2, float p3, float t);
float ubSplinePrimePrime(float p0, float p1, float p2, float p3, float t);

void cross(const vector3* v1, const vector3* v2, vector3* result);
void normalise(vector3* v);
void scale(vector3*v, float s);
void vecAdd(const vector3* v1, vector3* v2);

#endif
