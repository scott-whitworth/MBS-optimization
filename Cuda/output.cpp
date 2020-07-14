#include "output.h"
#include "constants.h"
#include "Thrust_files/thruster.h"
#include <string>
#include <iomanip>

// input: cConstants - access time_seed to derive file name
// output: mutateFile[time_seed].csv is given a header row, now ready to be used for progressiveRecord()
void setMutateFile(const cudaConstants* cConstants) { 
  std::ofstream mutateFile;
  mutateFile.open("mutateFile" + std::to_string(cConstants->time_seed) + ".csv", std::ios_base::app);

  mutateFile << "gen,";
  for (int i = 0; i < GAMMA_ARRAY_SIZE) {
    mutateFile << "gamma" << i << ",";
  }
  for (int i = 0; i < TAU_ARRAY_SIZE; i++) {
    mutateFile << "tau" << i << ",";
  }
  for (int i = 0; i < COAST_ARRAY_SIZE; i++) {
    mutateFile << "coast" << i << ",";
  }
  mutateFile << "alpha,beta,zeta,tripTime,\n";

  mutateFile.close();

}

// Output final results of genetic algorithm
// input: x[] - array that holds parameter values of OPTIM_VARS length
//        lastStep - Used to store number of total steps as output
//        threadRank - Rank of the individual element being recorded, currently (July )
//        yOut - Used to store element informatio as output
//        thrust - Passed into rk4sys
//        cConstants - Access constants info such as target element, earth element, derive spaceCraft element, also other values such as rk_tol
// output: yOut contains final eleement information of the spacecraft
//         lastStep contains value last value for number of steps taken
void trajectoryPrint( double x[], double & lastStep, int threadRank, elements<double> & yOut, thruster<double> thrust, const cudaConstants* cConstants) {
  /*set the asteroid and inital conditions for the earth and spacecraft:
  constructor takes in radial position(au), angluar position(rad), axial position(au),
  radial velocity(au/s), tangential velocity(au/s), axial velocity(au/s)*/

  // setting landing conditions of the asteroid (Sept 30, 2022)
  elements<double> asteroid = elements<double>(cConstants->r_fin_ast, cConstants->theta_fin_ast, cConstants->z_fin_ast, cConstants->vr_fin_ast, cConstants->vtheta_fin_ast, cConstants->vz_fin_ast);

  // setting initial conditions of earth based off of the impact date (Sept 30, 2022) minus the trip time (optimized).
  elements<double> earth =  launchCon->getCondition(x[TRIPTIME_OFFSET]);
  
  // setting initial conditions of the spacecraft
  elements<double> spaceCraft = elements<double>(earth.r+ESOI*cos(x[ALPHA_OFFSET]),
                                                 earth.theta + asin(sin(M_PI-x[ALPHA_OFFSET])*ESOI/earth.r),
                                                 earth.z,
                                                 earth.vr + cos(x[ZETA_OFFSET])*sin(x[BETA_OFFSET])*cConstants->v_escape,
                                                 earth.vtheta + cos(x[ZETA_OFFSET])*cos(x[BETA_OFFSET])*cConstants->v_escape,
                                                 earth.vz + sin(x[ZETA_OFFSET])*cConstants->v_escape);

  // setting time parameters
  double timeInitial=0; 
  double timeFinal=orbitalPeriod; // Orbital period of asteroid(s)
  double deltaT; // time step
  int numSteps = 5000; // initial guess for the number of time steps, guess for the memory allocated 
  deltaT = (timeFinal-timeInitial) / cConstants->max_numsteps; // initial guess for time step, small is preferable

  // setup of thrust angle calculations based off of optimized coefficients
  coefficients<double> coeff;
  initCoefficient(x,coeff, cConstants);
  // Assigning coast threshold (now done in coefficients because is a constant)

  // Assigning wetMass
  double wetMass = cConstants->wet_mass;
  // setting Runge-Kutta tolerance
  double absTol = cConstants->rk_tol;
  // set optmization minimum
  // double Fmin = cConstants->f_min;

  // Initialize memory for the solution vector of the dependant solution
  elements<double>* yp;
  yp = new elements<double>[numSteps];
  
  double *times, *gamma, *tau, *accel_output, *fuelSpent;
  times = new double[numSteps]; // Initialize memory for time array
  gamma = new double[numSteps]; // Initialize memory for gamma array
  tau = new double[numSteps]; // Initialize memory for tau array
  accel_output = new double[numSteps]; // Initialize memory for acceleration array
  fuelSpent = new double[numSteps];  // Initialize memory for fuelSpent array

  double accel; // Initialize memory for  acceleration

  // used to track the cost function throughout a run via output and outputs to a binary
  int lastStepInt;

  rk4sys(timeInitial, x[TRIPTIME_OFFSET] , times, spaceCraft, deltaT, yp, absTol, coeff, accel, gamma, tau, lastStepInt, accel_output, fuelSpent, wetMass, thrust, cConstants);

  lastStep = lastStepInt;

  // gets the final y values of the spacecrafts for the cost function.
  yOut = yp[(int)lastStep];

  std::ofstream output;
  double seed = cConstants->time_seed;
  output.open("orbitalMotion-"+std::to_string(static_cast<int>(seed))+".bin", std::ios::binary);
  // output.open("orbitalMotion-"+std::to_string(static_cast<int>(seed))+"-"+std::to_string(threadRank)+".bin", std::ios::binary);
  for(int i = 0; i <= lastStep; i++) {
    //output << yp[i];
    output.write((char*)&yp[i], sizeof (elements<double>));
    output.write((char*)&times[i], sizeof (double));
    output.write((char*)&gamma[i], sizeof (double));
    output.write((char*)&tau[i], sizeof (double));
    output.write((char*)&accel_output[i], sizeof (double));
    output.write((char*)&fuelSpent[i], sizeof (double));
  }
  output.close();
  
  // cleaning up dynamic yp, time, gamma, and tau.
  delete [] yp;
  delete [] times;
  delete [] gamma;
  delete [] tau;
}

// Output trajectory information to finalOptimization-[time_seed].bin
// input: start - passed to trajectoryPrint, information also outputted to finalOptimization[-time_seed].bin file
//        threadRank - passed to trajectoryPrint, also 
//        thrust - passed to trajectoryPrint
//        cConstants - access time_seed for deriving file name
// output: file finalOptimization[-time_seed].bin is created that holds earth/ast/ and trajectory parameter values
void writeTrajectoryToFile(double *start, int threadRank, thruster<double> thrust, const cudaConstants* cConstants) {
    elements<double> yp;
    double numStep = 0;
    trajectoryPrint(start, numStep, threadRank, yp, thrust, cConstants);

    //writes final optimization values to a seperate file
    std::ofstream output;
    double seed = cConstants->time_seed;
    output.open("finalOptimization-"+std::to_string(static_cast<int>(seed))+".bin", std::ios::binary);
    // output.open ("finalOptimization-"+std::to_string(static_cast<int>(seed))+"-"+std::to_string(threadRank)+".bin", std::ios::binary);

    output.write((char*)&cConstants->r_fin_ast, sizeof(double));
    output.write((char*)&cConstants->theta_fin_ast, sizeof(double));
    output.write((char*)&cConstants->z_fin_ast, sizeof(double));
    output.write((char*)&cConstants->vr_fin_ast, sizeof(double));
    output.write((char*)&cConstants->vtheta_fin_ast, sizeof(double));
    output.write((char*)&cConstants->vz_fin_ast, sizeof(double));
    output.write((char*)&cConstants->r_fin_earth, sizeof(double));
    output.write((char*)&cConstants->theta_fin_earth, sizeof(double));
    output.write((char*)&cConstants->z_fin_earth, sizeof(double));
    output.write((char*)&cConstants->vr_fin_earth, sizeof(double));
    output.write((char*)&cConstants->vtheta_fin_earth, sizeof(double));
    output.write((char*)&cConstants->vz_fin_earth, sizeof(double));
    output.write((char*)&cConstants->coast_threshold, sizeof(double));
    double gsize = GAMMA_ARRAY_SIZE, tsize = TAU_ARRAY_SIZE, csize = COAST_ARRAY_SIZE;
    output.write((char*)&gsize, sizeof(double));
    output.write((char*)&tsize, sizeof(double));
    output.write((char*)&csize, sizeof(double));

    for (int j = 0; j < OPTIM_VARS; j++) {
      output.write((char*)&start[j], sizeof (double));
    }
    
    output.write((char*)&numStep, sizeof (double));

  output.close();
}

// Called if record_mode is true at end of optimize process
// input: cConstants - access thruster_type info, best_count, and for recording the seed
//        generation - record the value
//        pool - accessing Individuals (top best_count ones)
//        start - passed to writeTrajectoryToFile method
//        thrust - 
// output: progressiveAnalysis.csv file is appended header information, followed by writeTrajectoryToFile and progressiveAnalysis called for best_count individuals
void progressiveRecord(const cudaConstants * cConstants, double generation, Individual * pool, double * start, thruster<double>& thrust) {
  std::ofstream progressiveOutput;
  progressiveOutput.open("progressiveAnalysis.csv", std::ios::app);
  progressiveOutput << std::endl << "seed:," << cConstants->time_seed << ",  ,generations:," << static_cast<int>(generation) << std::endl;
  progressiveOutput << "rank,posDiff (au),velDiff (au/s),tripTime (s),alpha (rad),beta (rad),zeta (rad),";

  if (cConstants->thruster_type == thruster<double>::NO_THRUST) {
      progressiveOutput << "gamma_a0,gamma_a1,gamma_b1,gamme_a2,gamme_b2,gamma_a3,gamma_b3,";
      progressiveOutput << "tau_a0,tau_a1,tau_b1,";
      progressiveOutput << "coast_a0,coast_a1,coast_b1,coast_a2,coast_b2,";
  }

  progressiveOutput << std::endl;
  // Write the best individuals with best_count in total outputted in seperate binary files
  for (int i = 0; i < cConstants->best_count; i++) {
      for (int j = 0; j < pool[i].startParams.coeff.gammaSize; j++) {
          start[GAMMA_OFFSET + j] = pool[i].startParams.coeff.gamma[j];
      }
      for (int j = 0; j < pool[i].startParams.coeff.tauSize; j++) {
          start[TAU_OFFSET + j] = pool[i].startParams.coeff.tau[j];
      }
      for (int j = 0; j < pool[i].startParams.coeff.coastSize; j++) {
          start[COAST_OFFSET + j] = pool[i].startParams.coeff.coast[j];
      }
      start[TRIPTIME_OFFSET] = pool[i].startParams.tripTime;
      start[ALPHA_OFFSET] = pool[i].startParams.alpha;
      start[BETA_OFFSET] = pool[i].startParams.beta;
      start[ZETA_OFFSET] = pool[i].startParams.zeta;
      // could instead use a ratio between position and velocity differnce as done in comparison of Individuals
      writeTrajectoryToFile(start, i+1, thrust, cConstants);
      progressiveAnalysis(progressiveOutput, i+1, pool[i], cConstants);
  }
  progressiveOutput << std::endl;
  progressiveOutput.close();
}

// Record progress of individual
// input: output - the output file stream being used
//        rank - the positional performance of the individual
//        ind - the individual object being recorded
//        config - cudaConstants object for accessing thruster_type information
// output: output file is appended information on rank, individual values/parameter information
void progressiveAnalysis(std::ofstream & output, int rank, Individual & ind, const cudaConstants* config) {
    output << rank << ',' << ind.posDiff << ',' << ind.velDiff << ',' << ind.startParams.tripTime << ',';
    output << ind.startParams.alpha << ',' << ind.startParams.beta << ',' << ind.startParams.zeta << ',';
    if (config->thruster_type) {
      for (int i = 0; i < GAMMA_ARRAY_SIZE; i++) {
        output << ind.startParams.coeff.gamma[i] << ',';
      }
      for (int i = 0; i < TAU_ARRAY_SIZE; i++) {
        output << ind.startParams.coeff.tau[i] << ',';
      }
      for (int i = 0; i < COAST_ARRAY_SIZE; i++) {
        output << ind.startParams.coeff.coast[i] << ',';
      }
    }
    output << std::endl;
}

// Initialize the .csv files
// input: cConstants - to access time_seed for deriving file name conventions and also thruster type
// output: files BestInGenerations-[time_seed].csv, WorstInGenerations-[time_seed].csv, if thruster type is not NO_THRUST also BestThrustGens-[time_seed].csv & WorstThrustGens-[time_seed].csv, are given initial header row info
void initializeRecord(const cudaConstants * cConstants) {
    // setup output of generation results over time onto a .csv file
    std::ofstream bestExcel;
    bestExcel.open("BestInGenerations-"+ std::to_string(cConstants->time_seed)+".csv");
    // Set first row in the file be a header for the columns
    bestExcel << "Gen #" << "," << "posDiff" << "," << "velDiff" << "," << "rFinal" << "," << "thetaFinal" << "," << "zFinal" << "," << "vrFinal"
              << "," << "vthetaFinal" << "," << "vzFinal" << "," << "rInitial" << "," << "thetaInitial" << "," << "zInitial" << ","<< "vrInitial"
              << "," << "vthetaInitial" << "," << "vzInitial" << "," << "alpha" << "," << "beta" << "," << "zeta" << "," << "anneal" << "," << "tripTime" << "\n";
    bestExcel.close();

    std::ofstream worstExcel;
    worstExcel.open("WorstInGenerations-"+ std::to_string(cConstants->time_seed)+".csv");
    // Set first row in the file be a header for the columns
    worstExcel << "Gen #" << "," << "posDiff" << "," << "velDiff" << "," << "rFinal" << "," << "thetaFinal" << "," << "zFinal" << "," << "vrFinal" 
               << "," << "vthetaFinal" << "," << "vzFinal" << "," << "rInitial" << "," << "thetaInitial" << "," << "zInitial" << ","<< "vrInitial" 
               << "," << "vthetaInitial" << "," << "vzInitial" << "," << "alpha" << "," << "beta" << "," << "zeta" << "," << "anneal" << "," << "tripTime" << "\n";
    worstExcel.close();

    if (cConstants->thruster_type != thruster<double>::NO_THRUST) {
        std::ofstream thrustBestExcel, thrustWorstExcel;

        thrustBestExcel.open("BestThrustGens-"+ std::to_string(cConstants->time_seed)+".csv");
        thrustBestExcel << "gen,";
        for (int i = 0; i < GAMMA_ARRAY_SIZE) {
          thrustBestExcel << "gamma" << i << ",";
        }
        for (int i = 0; i < TAU_ARRAY_SIZE; i++) {
          thrustBestExcel << "tau" << i << ",";
        }
        for (int i = 0; i < COAST_ARRAY_SIZE; i++) {
          thrustBestExcel << "coast" << i << ",";
        }
        thrustBestExcel << "\n";
        thrustBestExcel.close();

        thrustWorstExcel.open("WorstThrustGens-"+ std::to_string(cConstants->time_seed)+".csv");
        thrustWorstExcel << "gen,";
        for (int i = 0; i < GAMMA_ARRAY_SIZE) {
          thrustWorstExcel << "gamma" << i << ",";
        }
        for (int i = 0; i < TAU_ARRAY_SIZE; i++) {
          thrustWorstExcel << "tau" << i << ",";
        }
        for (int i = 0; i < COAST_ARRAY_SIZE; i++) {
          thrustWorstExcel << "coast" << i << ",";
        }
        thrustWorstExcel << "\n";
        thrustBestExcel.close();
    }
    // call setMutateFile to set it up
    setMutateFile(cConstants);
}

// Take in the current state of the generation and appends to files
// input: cConstants - access time_seed to derive file name
//        pool - passes pool[0] to writeIndividualToFiles & writeThrustToFiles
//        generation - passed to writeIndividualToFiles & writeThrustToFiles
//        new_anneal - passed into writeIndividualToFiles
//        poolSize - used to access worst individual in pool by using index value poolSize-1
//        thrust - used in conditional statement of thrust type
// output: files BestInGenerations/WorstInGenerations have appended information using writeIndividualToFiles method
//         files BestThrustGens/WorstThurstGens have appended information using writeThrustFiles method
void recordGenerationPerformance(const cudaConstants * cConstants, Individual * pool, double generation, double new_anneal, int poolSize, thruster<double>& thrust) {
  // Record
  std::ofstream bestExcel, bestBin;
  bestExcel.open("BestInGenerations-"+ std::to_string(cConstants->time_seed)+".csv", std::ios_base::app);
  bestBin.open("BestInGenerations-"+ std::to_string(cConstants->time_seed)+".bin", std::ios_base::app);

  writeIndividualToFiles(bestExcel, bestBin, generation, pool[0], new_anneal);

  bestExcel.close();
  bestBin.close();

  std::ofstream worstExcel, worstBin;
  worstExcel.open("WorstInGenerations-"+ std::to_string(cConstants->time_seed)+".csv", std::ios_base::app);
  worstBin.open("WorstInGenerations-"+ std::to_string(cConstants->time_seed)+".bin", std::ios_base::app);

  writeIndividualToFiles(worstExcel, worstBin, generation, pool[poolSize-1], new_anneal);

  worstExcel.close();
  worstBin.close();

  if (cConstants->thruster_type != thruster<double>::NO_THRUST) {
    std::ofstream bestThrusterExcel, bestThrusterBin;
    bestThrusterExcel.open("BestThrustGens-"+ std::to_string(cConstants->time_seed)+".csv", std::ios_base::app);
    bestThrusterBin.open("BestThurstGens-"+ std::to_string(cConstants->time_seed)+".bin", std::ios_base::app);
    
    writeThrustToFiles(bestThrusterExcel, bestThrusterBin, generation, pool[0], cConstants);
    
    bestExcel.close();
    bestBin.close();

    std::ofstream worstThrusterExcel, worstThrusterBin;
    worstExcel.open("WorstThrustGens-"+ std::to_string(cConstants->time_seed)+".csv", std::ios_base::app);
    worstBin.open("WorstThrustGens-"+ std::to_string(cConstants->time_seed)+".bin", std::ios_base::app);

    writeThrustToFiles(worstThrusterExcel, worstThrusterBin, generation, pool[poolSize-1], cConstants);
    
    worstThrusterExcel.close();
    worstThrusterBin.close();
  }

}

// Method for doing recording information at the end of the optimization process
// input: cConstants - access record_mode, if record_mode == true then call progressiveRecord method, also passed into writeTrajectoryToFile method as well as progressiveRecord
//        pool - To access the best individual (pool[0])
//        generation - to record the generation value 
//        thrust - passed into progressiveRecord and writeTrajectoryToFile
// output: writeTrajectoryToFile is called, if in record_mode then progressiveRecord is called as well
void finalRecord(const cudaConstants* cConstants, Individual * pool, double generation, thruster<double>& thrust) {
  // To store parameter values and pass onto writeTrajectoryToFile
  double *start = new double[OPTIM_VARS];

  // If in record mode, call progressiveRecord to store things in progressive analaysis file
  if (cConstants->record_mode == true) {
    progressiveRecord(cConstants, generation, pool, start, thrust);
  }

  // Only output the final best individual
  for (int j = 0; j < pool[0].startParams.coeff.gammaSize; j++) {
      start[GAMMA_OFFSET + j] = pool[0].startParams.coeff.gamma[j];
  }
  for (int j = 0; j < pool[0].startParams.coeff.tauSize; j++) {
      start[TAU_OFFSET + j] = pool[0].startParams.coeff.tau[j];
  }
  for (int j = 0; j < pool[0].startParams.coeff.coastSize; j++) {
      start[COAST_OFFSET + j] = pool[0].startParams.coeff.coast[j];
  }

  start[TRIPTIME_OFFSET] = pool[0].startParams.tripTime;
  start[ALPHA_OFFSET] = pool[0].startParams.alpha;
  start[BETA_OFFSET] = pool[0].startParams.beta;
  start[ZETA_OFFSET] = pool[0].startParams.zeta;

  // Could instead use a ratio between position and velocity differnce as done in comparison of Individuals
  writeTrajectoryToFile(start, 1, thrust, cConstants);

  delete start;
}

// Utility function to observe the trend of best individual in the algorithm through the generations
// Input: Two ofstreams (one to .csv file and another to binary), current generation number, best individual, and annealing value derived to be used in next generation crossover/mutation
// Output: The two streams are appended the individual's information and anneal value
void writeIndividualToFiles(std::ofstream& ExcelOutput, std::ofstream& BinOutput, double &currentGeneration, Individual &individual, double& annealing ) {
    // Output the information to excel spreadsheet
    ExcelOutput << currentGeneration << ','
                << individual.posDiff << ',' << individual.velDiff << ',' // The positional and velocity difference
                << individual.finalPos.r << ',' << individual.finalPos.theta << ',' << individual.finalPos.z << ',' // Final position
                << individual.finalPos.vr << ',' << individual.finalPos.vtheta << ',' << individual.finalPos.vz << ',' // Final velocity
                << individual.startParams.y0.r << ',' << individual.startParams.y0.theta << ',' << individual.startParams.y0.z << ',' // Starting position
                << individual.startParams.y0.vr << ',' << individual.startParams.y0.vtheta << ',' << individual.startParams.y0.vz << ',' // Starting velocity
                << individual.startParams.alpha << ',' << individual.startParams.beta << ',' << individual.startParams.zeta << ',' // alpha, beta, zeta
                << annealing << "," << individual.startParams.tripTime << std::endl; // Annealing value for next generation and triptime (in that order to maintain continuity with bin file)
 
    // Output the information to binary file for use in the MATLAB code, line breaks and spaces added to help with readibility
    BinOutput.write( (char*)& currentGeneration, sizeof(double));
    // posDiff and velDiff
    BinOutput.write( (char*)& individual.posDiff, sizeof(double));
    BinOutput.write( (char*)& individual.velDiff, sizeof(double));
    // Position and velocity information
    BinOutput.write( (char*)& individual.finalPos.r,            sizeof(double));
    BinOutput.write( (char*)& individual.finalPos.theta,        sizeof(double));
    BinOutput.write( (char*)& individual.finalPos.z,            sizeof(double));
    BinOutput.write( (char*)& individual.finalPos.vr,           sizeof(double));
    BinOutput.write( (char*)& individual.finalPos.vtheta,       sizeof(double));
    BinOutput.write( (char*)& individual.finalPos.vz,           sizeof(double));
    BinOutput.write( (char*)& individual.startParams.y0.r,      sizeof(double));
    BinOutput.write( (char*)& individual.startParams.y0.theta,  sizeof(double));
    BinOutput.write( (char*)& individual.startParams.y0.z,      sizeof(double));
    BinOutput.write( (char*)& individual.startParams.y0.vr,     sizeof(double));
    BinOutput.write( (char*)& individual.startParams.y0.vtheta, sizeof(double));
    BinOutput.write( (char*)& individual.startParams.y0.vz,     sizeof(double));
    // Alpha, Beta, Zeta, Annealing, Triptime
    BinOutput.write( (char*)& individual.startParams.alpha,  sizeof(double));
    BinOutput.write( (char*)& individual.startParams.beta,   sizeof(double));
    BinOutput.write( (char*)& individual.startParams.zeta,   sizeof(double));
    BinOutput.write((char*)& annealing, sizeof(double));
    BinOutput.write((char*)& individual.startParams.tripTime, sizeof(double));
}

// Utility function to observe the trend of best individual's thruster information in the algorithm through the generations
// Input: Two ofstreams (one to .csv file and another to binary), current generation number, best individual, and annealing value derived to be used in next generation crossover/mutation
// Output: The two streams are appended the individual's thruster information and anneal value
void writeThrustToFiles(std::ofstream& ExcelOutput, std::ofstream& BinOutput, double &currentGeneration, Individual &individual, const cudaConstants * cConstants) {
    ExcelOutput << currentGeneration << ',';
    for (int i = 0; i < GAMMA_ARRAY_SIZE; i++) {
        ExcelOutput << individual.startParams.coeff.gamma[i] << ',';
    }
    for (int i = 0; i < TAU_ARRAY_SIZE; i++) {
        ExcelOutput << individual.startParams.coeff.tau[i] << ',';
    }
    for (int i = 0; i < COAST_ARRAY_SIZE; i++) {
        ExcelOutput << individual.startParams.coeff.coast[i] << ',';
    }
    ExcelOutput << std::endl;

    BinOutput.write((char*)&currentGeneration, sizeof(double));
    for (int i = 0; i < GAMMA_ARRAY_SIZE; i++) {
        BinOutput.write((char*)&individual.startParams.coeff.gamma[i], sizeof(double));
    }
    for (int i = 0; i < TAU_ARRAY_SIZE; i++) {
        BinOutput.write((char*)&individual.startParams.coeff.tau[i], sizeof(double));
    }
    for (int i = 0; i < COAST_ARRAY_SIZE; i++) {
        BinOutput.write((char*)&individual.startParams.coeff.coast[i], sizeof(double));
    }
}
