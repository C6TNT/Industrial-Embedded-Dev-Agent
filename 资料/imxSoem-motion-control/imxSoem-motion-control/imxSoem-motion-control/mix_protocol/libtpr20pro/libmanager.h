#ifndef LIBMANAGER_H
#define LIBMANAGER_H
#include "math.h"
#include "coordinatetrans.h"
#include "fsl_debug_console.h"
#define N_AXIS 5
#define TRANSPER_1000 1000.0
typedef struct VelPlanner_
{
  int velocity[5];
  int targetposition[5];
  int acc[5];
  int dec[5];
} velplan_;
typedef struct Motor_Parameter_
{
  int direction;
  int initDec;
  double transmission;
  int dectime;
  int acctime;
  double maxvelocity;
} motorparam_;
int filterMaxVelocity(double maxvelocity, int acctime, int dectime, int v, int targetdec, int *vel);
void motordecToCJointpos(int currentaxisdec[5], motorparam_ mp[5], double *joint);
void baseToSixEndlink(double *ida, double *spacePosture);
void sixToBaseEndlink(double endx, double endy, double endz, double r, double p, double y, double *axisdec);
int sinefittingPlan(int step, double stoptime, int initVel_i, double circleTime, double tm, double td, double vMax, double targetposition, int *nowp, int *nowvel);
int sinefittingPlan_movl(int step, double stoptime, double initVel_i, double circleTime, double tm, double td, double vMax, double targetposition, double *nowp, double *nowvel);
int trapezoidalPlanOnceTime_stopTime(int step, double stoptime, int initVel, double circleTime, double accTime, double decTime, double v, double targetposition, int *nowp, int *nowvel);
velplan_ move_J(int currentaxisdec[5], motorparam_ mp[5], double *spacepos, int vel);
#endif // LIBMANAGER_H
