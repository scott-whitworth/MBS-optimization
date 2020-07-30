#define _USE_MATH_DEFINES // for use of M_PI
#include "runge_kuttaCUDA.cuh"
#include "runge_kutta.h" // used for rkCalc() and distinguishableDifference()
#include "rkParameters.h" // the struct containing the values passed to rk4simple()
#include "../Thrust_Files/thruster.h"
#include "../Genetic_Algorithm/ga_crossover.h" // crossover()
#include <math.h>
#include <iostream>
#include <fstream> // for outputing to .csv file
#include <chrono>
#include <algorithm> // sort(), shuffle()
#include <random>


void callRK(const int numThreads, const int blockThreads, Individual *generation, double timeInitial, double stepSize, double absTol, double & calcPerS, const cudaConstants* cConstant) {
    
    cudaEvent_t kernelStart, kernelEnd;
    cudaEventCreate(&kernelStart);
    cudaEventCreate(&kernelEnd);

    Individual *devGeneration; 
    double *devTimeInitial;
    double *devStepSize;
    double *devAbsTol;
    cudaConstants *devCConstant;

    // allocate memory for the parameters passed to the device
    cudaMalloc((void**) &devGeneration, numThreads * sizeof(Individual));
    cudaMalloc((void**) &devTimeInitial, sizeof(double));
    cudaMalloc((void**) &devStepSize, sizeof(double));
    cudaMalloc((void**) &devAbsTol, sizeof(double));
    cudaMalloc((void**) &devCConstant, sizeof(cudaConstants));

    // copy values of parameters passed from host onto device
    cudaMemcpy(devGeneration, generation, numThreads * sizeof(Individual), cudaMemcpyHostToDevice);
    cudaMemcpy(devTimeInitial, &timeInitial, sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(devStepSize, &stepSize, sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(devAbsTol, &absTol, sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(devCConstant, cConstant, sizeof(cudaConstants), cudaMemcpyHostToDevice);
    

    // GPU version of rk4Simple()
    cudaEventRecord(kernelStart);
    rk4SimpleCUDA<<<(numThreads+blockThreads-1)/blockThreads,blockThreads>>>(devGeneration, devTimeInitial, devStepSize, devAbsTol, numThreads, devCConstant);
    cudaEventRecord(kernelEnd);

    // copy the result of the kernel onto the host
    cudaMemcpy(generation, devGeneration, numThreads * sizeof(Individual), cudaMemcpyDeviceToHost);
    
    // free memory from device
    cudaFree(devGeneration);
    cudaFree(devTimeInitial);
    cudaFree(devStepSize);
    cudaFree(devAbsTol);
    cudaFree(devCConstant);

    float kernelT;
    
    cudaEventSynchronize(kernelEnd);

    cudaEventElapsedTime(&kernelT, kernelStart, kernelEnd);
    
    calcPerS = numThreads / (kernelT / 1000.0); // how many times the Runge Kutta algorithm ran in the kernel per second
}

// seperate conditions are passed for each thread, but timeInitial, stepSize, and absTol are the same for every thread
__global__ void rk4SimpleCUDA(Individual *individuals, double *timeInitial, double *startStepSize, double *absTolInput, int n, const cudaConstants* cConstant) {
    int threadId = threadIdx.x + blockIdx.x * blockDim.x;
    if (threadId < n) {
        rkParameters<double> threadRKParameters = individuals[threadId].startParams; // get the parameters for this thread

        elements<double> curPos = threadRKParameters.y0; // start with the initial conditions of the spacecraft

        // storing copies of the input values
        double stepSize = *startStepSize;
        double absTol = *absTolInput;
        double curTime = *timeInitial;
        double startTime = *timeInitial;
        double curAccel = 0;

        elements<double> k1, k2, k3, k4, k5, k6, k7; // k variables for Runge-Kutta calculation of y based off the spacecraft's final state

        thruster<double> thrust(cConstant);

        double massFuelSpent = 0; // mass of total fuel expended (kg) starts at 0

        bool coast; // to hold the result from calc_coast()

        elements<double> error; // holds output of previous value from rkCalc

        while (curTime < threadRKParameters.tripTime) {

            if (cConstant->thruster_type == thruster<double>::NO_THRUST) {
                coast = curAccel = 0;
            }
            else {
                coast = calc_coast(threadRKParameters.coeff, curTime, threadRKParameters.tripTime, thrust);
                curAccel = calc_accel(curPos.r, curPos.z, thrust, massFuelSpent, stepSize, coast, static_cast<double>(cConstant->wet_mass), cConstant);
            }

            // calculate k values and get new value of y
            rkCalc(curTime, threadRKParameters.tripTime, stepSize, curPos, threadRKParameters.coeff, curAccel, error, k1, k2, k3, k4, k5, k6, k7); 

            curTime += stepSize; // update the current time in the simulation
            
            stepSize *= calc_scalingFactor(curPos-error,error,absTol,stepSize); // Alter the step size for the next iteration

            // The step size cannot exceed the total time divided by 2 and cannot be smaller than the total time divided by 1000
            if (stepSize > (threadRKParameters.tripTime - startTime) / cConstant->min_numsteps) {
                stepSize = (threadRKParameters.tripTime - startTime) / cConstant->min_numsteps;
            }
            else if (stepSize < (threadRKParameters.tripTime - startTime) / cConstant->max_numsteps) {
                stepSize = (threadRKParameters.tripTime - startTime) / cConstant->max_numsteps;
            }
            
            if ( (curTime + stepSize) > threadRKParameters.tripTime) {
                stepSize = (threadRKParameters.tripTime - curTime); // shorten the last step to end exactly at time final
            }

            // if the spacecraft is within 0.5 au of the sun, the radial position of the spacecraft artificially increases to 1000, to force that path to not be used in the optimization.
            if ( sqrt(pow(curPos.r,2) + pow(curPos.z,2)) < 0.5) {
                curPos.r = 1000;

                // output to this thread's index
                individuals[threadId].finalPos = curPos;
                individuals[threadId].posDiff = 1.0e10;
                individuals[threadId].velDiff =  0.0;

                return;
            }
        }

         // output to this thread's index
        individuals[threadId].finalPos = curPos;

        individuals[threadId].getPosDiff(cConstant);
        individuals[threadId].getVelDiff(cConstant);
        individuals[threadId].getCost(cConstant);

        return;
    }
    return;
}
    
bool distinguishableDifference(double p1, double p2, double distinguishRate) {

    double p1Num = trunc(p1/distinguishRate);
    double p2Num = trunc(p2/distinguishRate);
    
    std:: cout << "p1Num: " << p1Num << std::endl;
    std:: cout << "p2Num: " << p2Num << std::endl;

    if(p1Num == p2Num) {
        return true;
    } else {
        return false;
    }

}
