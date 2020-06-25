#ifndef orbitalMotion_h
#define orbitalMotion_h
#include "../Motion_Eqns/elements.h"
#include "../Thrust_Files/thruster.h"

// Input for earthInitial_incremental:
//      timeInitial: time (in seconds) of the impact date
//      tripTime: optimized time period of the overall trip
//      earth: earth's position and velocity on the impact date October 5, 2022 (passed in from earthInfo.cpp)
elements<double> earthInitial_incremental(double timeInitial, double tripTime,const elements<double> & earth);

double trajectoryPrint( double x[], double & lastStep, double & cost, int j, elements<double> & yOut, thruster<double> thrust, int runNumber);

void writeTrajectoryToFile(double *start, double & cost, int i, thruster<double> thrust, int runNumber);

#include "orbitalMotion.cpp"
#endif