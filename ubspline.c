#include <stdio.h>
#include <math.h>
#include "ubspline.h"

#define sixth 0.166666666

// Helper for q
typedef float (*splineFunc)(float, float, float, float, float);
splineFunc ubSplineFunctions[] = { &ubSpline, &ubSplinePrime, &ubSplinePrimePrime};
// Main function to get a b spline function.
int q(const vector3* list, float t, int derivative, vector3* result){
  int i;
  int index = t;
  float splineT = t - index;
  for(i = 0; i < 3; i++)
    result->a[i] = ubSplineFunctions[derivative](list[index-3].a[i],
                                                 list[index-2].a[i],
                                                 list[index-1].a[i],
                                                 list[index].a[i],
                                                 splineT);
  return 0;
}
// Acts as the normal b spline function.
float ubSpline(float p0, float p1, float p2, float p3, float t){
  float mt = 1 - t;
  float tt = t*t;
  float ttt= tt*t;
  return sixth * (mt * mt * mt * p0
       + ( 3*ttt - 6*tt + 4) * p1
       + (-3*ttt + 3*tt + 3*t + 1) * p2
       + ttt * p3);
}
// The first derivitive of the b spline function.
float ubSplinePrime(float p0, float p1, float p2, float p3, float t){
  float mt = 1 - t;
  float tt = t*t;
  return (-0.5 * mt * mt) * p0
       + (1.5 * tt -2 * t) * p1
       + (-1.5* tt + t + 0.5) * p2
       + (0.5 * tt) * p3;
}
// The second derivitive of the b spline function.
float ubSplinePrimePrime(float p0, float p1, float p2, float p3, float t){
  return (1 - t) * p0
       + (3*t - 2) * p1
       + (-3 * t + 1) * p2
       + t * p3;
}
// Cross multiplication of two vector3s.
 void cross(const vector3* v1, const vector3* v2, vector3* result)
 {
   result->x = v1->y * v2->z - v1->z * v2->y;
   result->y = v1->z * v2->x - v1->x * v2->z;
   result->z = v1->x * v2->y - v1->y * v2->x;
 }
// Should get the magnitude of a vector
float magnitude(const vector3* v)
{
  return sqrt((v->x * v->x) + (v->y * v->y) + (v->z * v->z));
}
// Normalizes a given vector.
 void normalise(vector3* v)
 {
   float mag = magnitude(v);
   scale(v, 1/mag);
 }
// Scales v by s.
void scale(vector3*v, float s)
{
  v->x *= s;
  v->y *= s;
  v->z *= s;
}
// Adds vector 1 to vector 2.
void vecAdd(const vector3* v1, vector3* v2)
{
  v2->x += v1->x;
  v2->y += v1->y;
  v2->z += v1->z;
}
