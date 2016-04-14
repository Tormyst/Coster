/*
 *	coaster.c
 *
 *  Main file for the roller coaster.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef __APPLE__
    #include <GLUT/glut.h> // Required on mac.  Was working with GL/glut.h in 10.10.5, but not in 10.11.2
#else
    #include <GL/glut.h> // Required elsewhere.
#endif

#include "ubspline.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD2DEG 180.0/M_PI
#define DEG2RAD M_PI/180.0
// The sky calls to us, this is some values to help it.  Also the ground is also using these values.
#define SKY_RAD 20
#define SKY_STEPS 40
#define SKY_HEIGHT 20
// 33 for 30 fps, 16 for 60.  Now,
#define RECALLTIME 33
// Default rotation speed.
#define ROTATE_SPEED 0.0005
// When should we put suports in the track?
#define TRACK_SUPORT_RANGE 20
// How large is the beams between tracks?
#define BEAM_RAD 0.03
// What is the distance between rails?
#define RAIL_DISTANCE 0.1

/* -- function prototypes --------------------------------------------------- */
static void	myDisplay(void);
static void	mainTime(int value);
static void	myReshape(int w, int h);
static void myKey(unsigned char key, int x, int y);
static void specialKey(int key, int x, int y);
static void	init(void);
static void coasterInit(void);
static void readcoaster(void);
static void precomputeTrack(void);
static void drawSkyAndGround(void);
static void drawBox(void);
static void drawSupport(float v1x, float v1y, float v1z, float v2x, float v2y, float v2z);
static void drawBar(float v1x, float v1y, float v1z, float v2x, float v2y, float v2z);
static void calculateUp(const vector3* forward, const vector3* s, vector3* up);
/* -- global variables ------------------------------------------------------ */
int xMax, yMax; // Window size.
char* rolercoasterFile; // Where is the roler coaster?  Will be set in main if you don't. pass a value to the program.
int controlPointCount; // First line of the file.
float rotation, zoom, deltaRotation = ROTATE_SPEED; // For rotating the camera in rotation mode.
vector3* coasterControlPoints; // The list of points read in from the file.
int listName; // The number of the list GL gives us.
float vCart, uCart, maxY = 0; // Stuff for moving the cart.  speed, position and max height.
vector3 cameraLocation, lookAtPoint, cameraUp; // Camera settings for the track cam.
int mode; // What mode are we in.  0 for rotation, 1 for coaster cam.
/* -- main ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
  glutInit(&argc, argv);
  // Set read in file.
  if(argc > 1){
    rolercoasterFile = argv[1];
  }
  else{
    rolercoasterFile = "wildbeast.rc";
  }

  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
  glutInitWindowSize(1200, 600);
  glutCreateWindow("Roler coaster Ride");
  glutDisplayFunc(myDisplay);
  glutKeyboardFunc(myKey);
  glutSpecialFunc(specialKey);
  glutReshapeFunc(myReshape);
  glutTimerFunc(RECALLTIME, mainTime, 0);

  init();

  glutMainLoop();

  return 0;
}

/* -- callback functions ---------------------------------------------------- */

void
myDisplay() // Display callback.
{
  int	i;
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  if(mode)
  {
    // Track cam.
    gluLookAt(cameraLocation.x,cameraLocation.y,cameraLocation.z,
            lookAtPoint.x, lookAtPoint.y, lookAtPoint.z,
            cameraUp.x, cameraUp.y, cameraUp.z);
  }
  else
  {
    // Rotation cam.
    gluLookAt(zoom*sin(rotation),8,zoom*cos(rotation),
            0, 0, 0,
            0, 1, 0);
  }
  // Build everything.
  glCallList(listName);

  glutSwapBuffers();
}

void
mainTime(int value)
{
  vector3 velocity, s;
  // Move the cart.
  q(coasterControlPoints, uCart, 1, &velocity);
  uCart += (vCart * (RECALLTIME * 0.001)) / (sqrt(velocity.x*velocity.x+velocity.y*velocity.y+velocity.z*velocity.z));
  if(uCart > controlPointCount + 3) uCart -= controlPointCount;
  // Calculate my up vector.
  q(coasterControlPoints, uCart, 0, &cameraLocation);
  q(coasterControlPoints, uCart, 2, &s);
  normalise(&velocity);
  calculateUp(&velocity, &s, &cameraUp);
  normalise(&cameraUp);
  // Set next velocity value.
  vCart = sqrt((maxY + maxY * 0.25) - cameraLocation.y);
  // I should probably look at the track insted of ahead.  Makes you feel like you are on it more.
  lookAtPoint = cameraLocation;
  lookAtPoint.x += velocity.x;
  lookAtPoint.y += velocity.y;
  lookAtPoint.z += velocity.z;
  // Camera probably should not be on the track.
  vecAdd(&cameraUp, &cameraLocation);

  // Lets not forget about rotation.
  rotation += deltaRotation * RECALLTIME;
  if(rotation > 2*M_PI)
    rotation -= 2*M_PI;

  glutPostRedisplay();
  glutTimerFunc(RECALLTIME, mainTime, value);		/* 30 frames per second */
}

// Deals with space, p and x.
void myKey(unsigned char key, int x, int y)
{
    switch (key) {
        case ' ': // Change modes with space.
          mode = !mode;
          break;
        case '=': // + will zoom in.
        case '+':
          zoom -= 0.5;
          break;
        case '-': // And - will zoom out.
        case '_':
          zoom += 0.5;
          break;
        case 'r': // Very broken, Be careful.
        case 'R': // Try and reload the coaster.  This has mixed results and works well enough.
          free(coasterControlPoints);
          coasterInit();
        default:
            break;
    }
}

void specialKey(int key, int x, int y)
{
  switch (key) {
    case GLUT_KEY_LEFT: // Stops rotation and move the rotation to the left.
      rotation -= 0.05;
      deltaRotation = 0;
      break;
    case GLUT_KEY_RIGHT: // Stops rotation and moves the rotation to the right.
      rotation += 0.05;
      deltaRotation = 0;
      break;
    case GLUT_KEY_DOWN: // Restart rotation.
      deltaRotation = ROTATE_SPEED;
      break;
  }
}

// Do what you want.  This looks good in any screen size.
void myReshape(int w, int h)
{
    xMax = 100.0*w/h;
    yMax = 100.0;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(100, xMax/yMax, 0.1, 50); // Why 100? Because you must be at leest this quake pro to ride.

    glMatrixMode(GL_MODELVIEW);
}

/* -- other functions ------------------------------------------------------- */
void init()
{
  // Init function.  sets up start things.
  glClearColor(0.86, 0.078, 0.235, 1.0);
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glLineWidth(3);
  glEnable(GL_DEPTH_TEST);
  glPointSize(15);
  rotation = 0.0;
  zoom = 10;
  vCart = 1;
  uCart = 3;
  mode = 0;
  listName = glGenLists(1);
  coasterInit();
}
// Gets the coaster ready.
void coasterInit()
{
  readcoaster();
  precomputeTrack();
}
// Reads in the coaster from file, and sets controlPointCount and coasterControlPoints.
// For some reason this function has a hard time sometimes.  Just try it again if it is not working.
void readcoaster()
{
  int i;
  FILE* file = fopen(rolercoasterFile, "r");
  if(file == NULL){
    printf("Could not open file %s\n", rolercoasterFile);
    exit(-1);
  }
  fscanf(file, " %d \n", &controlPointCount);
  coasterControlPoints = (vector3*) malloc((controlPointCount + 3)*sizeof(vector3));
  for(i = 0; i < controlPointCount; i++){
    fscanf(file, "%f %f %f\n", &coasterControlPoints[i].x, &coasterControlPoints[i].y, &coasterControlPoints[i].z);
    if(coasterControlPoints[i].y > maxY)
      maxY = coasterControlPoints[i].y;
  }

  coasterControlPoints[controlPointCount] = coasterControlPoints[0];
  coasterControlPoints[controlPointCount+1] = coasterControlPoints[1];
  coasterControlPoints[controlPointCount+2] = coasterControlPoints[2];
  fclose(file);
}
// This sets up the display list with everything it needs.
void precomputeTrack()
{
  float currentLocation = 3, stepSize;
  vector3 point, lastPoint, firstPoint, forward, right, up, s, lastRight, firstRight;
  int i = 0;

  glNewList(listName, GL_COMPILE);

  drawSkyAndGround();

//  glColor3f(0,0,1);
//  glBegin(GL_POINTS);
//  for(i = 0; i < controlPointCount; i++)
//    glVertex3fv(&coasterControlPoints[i]);
//  glEnd();

  i = 0;
  while(currentLocation < controlPointCount+3)
  {
    q(coasterControlPoints, currentLocation, 0, &point);
    q(coasterControlPoints, currentLocation, 1, &forward);
    // Before we normalise forward we need to check this. Its how much we will step next.
    stepSize = RAIL_DISTANCE / (sqrt(forward.x*forward.x+forward.y*forward.y+forward.z*forward.z));
    q(coasterControlPoints, currentLocation, 2, &s);
    normalise(&forward);
    normalise(&s);

    calculateUp(&forward, &s, &up);

    cross(&forward, &up, &right);
    // how far the tracks are from each other.  right should already be normalised as both forward and up are.
    scale(&right, 0.4);

    glColor3f(0,0,0);
    glPushMatrix();
    glTranslatef(point.x, point.y, point.z);
    // I actualy like the cubes like this.  I tried without them, and it just kind of looked sad.
    glPushMatrix();
    glTranslatef(right.x,right.y,right.z);
    glScalef(0.03, 0.03, 0.03);
    drawBox();
    glPopMatrix();

    glColor3f(1,1,1);
    glPushMatrix();
    glTranslatef(-right.x,-right.y,-right.z);
    glScalef(0.03, 0.03, 0.03);
    drawBox();
    glPopMatrix();

    glPopMatrix();
    // Line in between rails on each track.
    if(i % 2 == 0) glColor3f(0,0,0);
    drawBar(point.x-right.x, point.y-right.y, point.z-right.z,
             point.x+right.x, point.y+right.y, point.z+right.z);
    // Line between tracks.
    if( i > 0 )
    { // I thought drawBar would work better here, but this looked better.  So I am sticking with this.
      glColor3f(0,0,0);
      drawSupport(lastPoint.x-lastRight.x, lastPoint.y-lastRight.y, lastPoint.z-lastRight.z,
               point.x-right.x, point.y-right.y, point.z-right.z);
      glColor3f(1,1,1);
      drawSupport(lastPoint.x+lastRight.x, lastPoint.y+lastRight.y, lastPoint.z+lastRight.z,
               point.x+right.x, point.y+right.y, point.z+right.z);
    }
    else
    {
      firstRight = right;
      firstPoint = point;
    }
    // Draws vertical lines for suports.
    if(i % TRACK_SUPORT_RANGE == 0)
    {
      glColor3f(0,0,0);
      drawSupport(point.x-right.x, point.y-right.y, point.z-right.z,
               point.x-right.x, 0, point.z-right.z);
      glColor3f(1,1,1);
      drawSupport(point.x+right.x, point.y+right.y, point.z+right.z,
               point.x+right.x, 0, point.z+right.z);
    }
    lastRight = right;
    lastPoint = point;
    // Because sometimes this was a problem.  And we have a minimum.
    if(stepSize < 0.01) stepSize = 0.01;
    currentLocation += stepSize;
    i++;
  }
  // Just to make a circle.
  glColor3f(0,0,0);
  drawSupport(lastPoint.x-lastRight.x, lastPoint.y-lastRight.y, lastPoint.z-lastRight.z,
           firstPoint.x-firstRight.x, firstPoint.y-firstRight.y, firstPoint.z-firstRight.z);
  glColor3f(1,1,1);
  drawSupport(lastPoint.x+lastRight.x, lastPoint.y+lastRight.y, lastPoint.z+lastRight.z,
           firstPoint.x+firstRight.x, firstPoint.y+firstRight.y, firstPoint.z+firstRight.z);

  glEndList();
}

/* -- helper functions ------------------------------------------------------ */
// Well, this did start off as green with blue sky, then something happend...
// My high contrast track felt like it was from beetlejuice, and so...
void drawSkyAndGround(){
  int i;
  float step = (2*M_PI)/SKY_STEPS;
  float x,z;
  glBegin(GL_QUAD_STRIP);
    for(i = 0; i < SKY_STEPS+1; i++){
      x = sin(step*i)*SKY_RAD;
      z = cos(step*i)*SKY_RAD;
      glColor3f(0, 0, 0);
      glVertex3f(x, -SKY_HEIGHT, z);
      glColor3f(0.86, 0.078, 0.235);
      glVertex3f(x, SKY_HEIGHT, z);
    }
  glEnd();
  glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.1,0.1,0);
    glVertex3d(0,0,0);
    glColor3f(0.694,0.612,0.85);
    for(i=0; i < SKY_STEPS + 1; i++){
      glVertex3f(sin(step*i)*SKY_RAD, 0, cos(step*i)*SKY_RAD);
    }
  glEnd();
}
// Look out, its a box.
void drawBox(){
  glBegin(GL_QUADS);
    glVertex3f(1,1,1);
    glVertex3f(-1,1,1);
    glVertex3f(-1,1,-1);
    glVertex3f(1,1,-1);

    glVertex3f(1,-1,1);
    glVertex3f(-1,-1,1);
    glVertex3f(-1,-1,-1);
    glVertex3f(1,-1,-1);

    glVertex3f(1,1,1);
    glVertex3f(1,-1,1);
    glVertex3f(1,-1,-1);
    glVertex3f(1,1,-1);

    glVertex3f(-1,1,1);
    glVertex3f(-1,-1,1);
    glVertex3f(-1,-1,-1);
    glVertex3f(-1,1,-1);

    glVertex3f(1,1,-1);
    glVertex3f(1,-1,-1);
    glVertex3f(-1,-1,-1);
    glVertex3f(-1,1,-1);

    glVertex3f(1,1,1);
    glVertex3f(1,-1,1);
    glVertex3f(-1,-1,1);
    glVertex3f(-1,1,1);
  glEnd();
}
// We draw a beam that should only work in the y direction, but it still looks good for the track..
// I still don't get it.
void drawSupport(float v1x, float v1y, float v1z, float v2x, float v2y, float v2z){
  glBegin(GL_QUAD_STRIP);
    glVertex3f(v1x+BEAM_RAD,v1y,v1z);
    glVertex3f(v2x+BEAM_RAD,v2y,v2z);
    glVertex3f(v1x,v1y,v1z+BEAM_RAD);
    glVertex3f(v2x,v2y,v2z+BEAM_RAD);
    glVertex3f(v1x-BEAM_RAD,v1y,v1z);
    glVertex3f(v2x-BEAM_RAD,v2y,v2z);
    glVertex3f(v1x,v1y,v1z-BEAM_RAD);
    glVertex3f(v2x,v2y,v2z-BEAM_RAD);
    glVertex3f(v1x+BEAM_RAD,v1y,v1z);
    glVertex3f(v2x+BEAM_RAD,v2y,v2z);
  glEnd();
}
// My atempt at a rotation on a bar.  It kind of works, I guess.
void drawBar(float v1x, float v1y, float v1z, float v2x, float v2y, float v2z){

  vector3 dir, tmp={{0,1,0}} ,off1, off2;

  dir.x = v2x - v1x;
  dir.y = v2y - v1y;
  dir.z = v2z - v1z;

  cross(&dir, &tmp, &off1);
  tmp.x = 1;
  tmp.y = 0;
  cross(&dir, &tmp, &off2);

  glBegin(GL_QUAD_STRIP);
    glVertex3f(v1x+BEAM_RAD*off1.x,v1y+BEAM_RAD*off1.y,v1z+BEAM_RAD*off1.z);
    glVertex3f(v2x+BEAM_RAD*off1.x,v2y+BEAM_RAD*off1.y,v2z+BEAM_RAD*off1.z);
    glVertex3f(v1x+BEAM_RAD*off2.x,v1y+BEAM_RAD*off2.y,v1z+BEAM_RAD*off2.z);
    glVertex3f(v2x+BEAM_RAD*off2.x,v2y+BEAM_RAD*off2.y,v2z+BEAM_RAD*off2.z);
    glVertex3f(v1x-BEAM_RAD*off1.x,v1y-BEAM_RAD*off1.y,v1z-BEAM_RAD*off1.z);
    glVertex3f(v2x-BEAM_RAD*off1.x,v2y-BEAM_RAD*off1.y,v2z-BEAM_RAD*off1.z);
    glVertex3f(v1x-BEAM_RAD*off2.x,v1y-BEAM_RAD*off2.y,v1z-BEAM_RAD*off2.z);
    glVertex3f(v2x-BEAM_RAD*off2.x,v2y-BEAM_RAD*off2.y,v2z-BEAM_RAD*off2.z);
    glVertex3f(v1x+BEAM_RAD*off1.x,v1y+BEAM_RAD*off1.y,v1z+BEAM_RAD*off1.z);
    glVertex3f(v2x+BEAM_RAD*off1.x,v2y+BEAM_RAD*off1.y,v2z+BEAM_RAD*off1.z);
  glEnd();
}
// Calculates tilt and then tries an applies that to the unit y vector.
void calculateUp(const vector3* forward, const vector3* s, vector3* up)
{
  float cosVal, sinVal;
  float k = (forward->z*s->x - forward->x-s->z)/pow(forward->x*forward->x+forward->y*forward->y+forward->z*forward->z,1.5);
  k = k / 8;
  if(k > M_PI/2)
    k = M_PI/2;
  else if(k < -M_PI/2)
    k = -M_PI/2;

  cosVal = cos(-k);
  sinVal = sin(-k);
  up->x = (1-cosVal)*forward->x*forward->y-sinVal*forward->z;
  up->y = (1-cosVal)*forward->y*forward->y+cosVal;
  up->z = (1-cosVal)*forward->y*forward->z+sinVal*forward->x;
  normalise(up);
}
