// Collection of functions to perform crossover operations on rkParameters
// A crossover mask is an array of elements equal to the number of elements in the rkParameters list
// The mask defines what elements come from partent n and parent m
// [ {gamma}    {tau}       {alpha}     {beta}      {zeta}      {tripTime}      {coast} ]
// [   0-6       7-9           10         11          12            13           14-18  ]                                         ]

#include "../Runge_Kutta/rkParameters.h"
#include "../Config_Constants/config.h"
#include "ga_crossover.h"
#include <iostream>
#include <chrono>

#define SECONDS_IN_YEAR 365*24*3600 // Used with getRand for triptime mutation scale


enum maskValue {
    PARTNER1 = 1,
    PARTNER2,
    AVG,
};


// Creates a random bifurcation mask
// Randomly picks one index to be the start of the '2's from mask
// input: rng - a constructed mt19937_64 random number generator
// in/out: mask - all data will be overwritten
//              - Based on random index, first selection will be 1's, last selection will be 2's
//              - ex: [1, 1, 1, 1, 2, 2]
void crossOver_randHalf(int mask[], std::mt19937_64 & rng) {
    int crossIndex = rng() % (OPTIM_VARS-1);
    //cout << "Random Index: " << crossIndex << endl;
    for (int i = 0; i < OPTIM_VARS; i++) {
        if (i > crossIndex) {
            mask[i] = 2;
        }
        else {
            mask[i] = 1;
        }        
    }
    return;
}

// Creates a random mask
// Each element in a mask is randomly set to either 1 or 2
// in/out : All data overwritten, set randomly
// input: rng - a constructed mt19937_64 random number generator
void crossOver_wholeRandom(int mask[], std::mt19937_64 & rng) {
    for (int i = 0; i < OPTIM_VARS; i++ ) {
        if (rng() % 2) { //Coin flip, either 1/0
            mask[i] = 2;
        }
        else {
            mask[i] = 1;
        }
    }
    return;
}

// This crossover method randomly chooses between partners for each variable. Similar to crossOver_wholeRandom, but this keeps all parameters for each variable together
void crossOver_bundleVars(int mask[], std::mt19937_64 & rng) {
    int p_gamma = 1 + rng() % 2; // partners for each variable are randomly chosen between 1 and 2
    int p_tau = 1 + rng() % 2;
    int p_coast = 1 + rng() % 2;
    int p_triptime = 1 + rng() % 2;
    int p_alpha = 1 + rng() % 2;
    int p_beta = 1 + rng() % 2;
    int p_zeta = 1 + rng() % 2;

    // Gamma
    for (int i = GAMMA_OFFSET; i < (GAMMA_OFFSET + GAMMA_ARRAY_SIZE); i++) {
        mask[i] = p_gamma;
    }
    // Tau
    for (int i = TAU_OFFSET; i < (TAU_OFFSET + TAU_ARRAY_SIZE); i++) {
        mask[i] = p_tau;
    }
    // Coast
    for (int i = COAST_OFFSET; i < (COAST_OFFSET + COAST_ARRAY_SIZE); i++) {
        mask[i] = p_coast;
    }

    mask[TRIPTIME_OFFSET] = p_triptime;

    mask[ALPHA_OFFSET] = p_alpha;
    
    mask[BETA_OFFSET] = p_beta;
    
    mask[ZETA_OFFSET] = p_zeta;

    return;
}

void crossOver_average(int mask[]) {
    for (int i = 0; i < OPTIM_VARS; i++) {
        mask[i] = 3;
    }
    return;
}

// CURRENTLY NOT IN USE. THIS FUNCTION AND crossOver_tauPos WERE REPLACED BY crossOver_bundleVars
//Create a mask the flips just the gamma coefficients
// input mask: over writes all data with [1 ... 1, 2, 2, ... 2, 2, 1 ... 1]
// 2's correspond to Gamma coefficients
/*void crossOver_gammaPos(int mask[]) {
    for (int i = 0; i < OPTIM_VARS; i++) {
        if ( (i >= GAMMA_OFFSET) && (i < (GAMMA_OFFSET + GAMMA_ARRAY_SIZE) ) ) {
            mask[i] = 2;
        } 
        else {
            mask[i] = 1;
        }
    }
    return;
}*/

// CURRENTLY NOT IN USE. THIS FUNCTION AND crossOver_tauPos WERE REPLACED BY crossOver_bundleVars
//Create a mask the flips just the tau coefficients
// input mask: over writes all data with [1 ... 1, 2, 2, ... 2, 2, 1 ... 1]
// 2's correspond to tau coefficients
/*void crossOver_tauPos(int mask[]) {
    for (int i = 0; i < OPTIM_VARS; i++) {
        if ( (i >= TAU_OFFSET) && (i < (TAU_OFFSET + TAU_ARRAY_SIZE) ) ) {
            mask[i] = 2;
        }
        else {
            mask[i] = 1;
        }
    }
    return;
}*/

//Utility to flip the polarity of a mask
// input:  mask is an array of size OPTIM_VARS, input with either 1 or 2 as a mask
// output: each 1 in mask will be reassigned to be a 2, each 2 will be reassigned 1
void flipMask(int mask[]) {
    for (int i = 0; i < OPTIM_VARS; i++) {
        if (mask[i] == 1) {
            mask[i] = 2;
        }
        else {
            mask[i] = 1;
        }
    }
    return;
}

//Copy contents of maskIn into maskOut
void copyMask(int maskIn[], int maskOut[]) {
    for (int i = 0; i < OPTIM_VARS; i++) {
        maskOut[i] = maskIn[i];
    }
}

// Display mask contents onto the terminal
void printMask(int mask[]) {
    std::cout << "[";

    for(int i = 0; i < OPTIM_VARS; i++) {
        std::cout << mask[i];
        // If not the last item, need a comma to seperate between the items in the display
        if ( i < OPTIM_VARS-1 ) {
            std::cout <<", ";
        }
    }
    std::cout << "]";
}

rkParameters<double> generateNewIndividual(const rkParameters<double> & p1, const rkParameters<double> & p2, const int mask[], thruster<double>& thrust) {
    rkParameters<double> newInd = p1;

        
    // Only calculate values pertaining to thrust if a thruster is being used
    if (thrust.type != thruster<double>::NO_THRUST) {
        // Iterate through the gamma values
        for (int i = GAMMA_OFFSET; i < (GAMMA_OFFSET + GAMMA_ARRAY_SIZE); i++) {
            if (mask[i] == PARTNER2) {
                newInd.coeff.gamma[i - GAMMA_OFFSET] = p2.coeff.gamma[i - GAMMA_OFFSET]; // If the mask is 2, use the value from parent 2
            }
            else if (mask[i] == AVG) {
                newInd.coeff.gamma[i - GAMMA_OFFSET] = p2.coeff.gamma[i - GAMMA_OFFSET]/2.0 + p1.coeff.gamma[i - GAMMA_OFFSET]/2.0;// If the mask is 3, average the values from both parents
            }
        }
        // Iterate through tau values
        for (int i = TAU_OFFSET; i < (TAU_OFFSET + TAU_ARRAY_SIZE); i++) {
            if (mask[i] == PARTNER2) {
                newInd.coeff.tau[i - TAU_OFFSET] = p2.coeff.tau[i - TAU_OFFSET];
            }
            else if (mask[i] == AVG) {
                newInd.coeff.tau[i - TAU_OFFSET] = p2.coeff.tau[i - TAU_OFFSET]/2.0 + p1.coeff.tau[i - TAU_OFFSET]/2.0;
            }
        }
        // Iterate through coasting values
        for (int i = COAST_OFFSET; i < (COAST_OFFSET + COAST_ARRAY_SIZE); i++) {
            if (mask[i] == PARTNER2) {
                newInd.coeff.coast[i - COAST_OFFSET] = p2.coeff.coast[i - COAST_OFFSET];
            }
            else if (mask[i] == AVG) {
                newInd.coeff.coast[i - COAST_OFFSET] = p2.coeff.coast[i - COAST_OFFSET]/2.0 + p1.coeff.coast[i - COAST_OFFSET]/2.0;
            }
        }
    }
    // Go through other variables
    if (mask[TRIPTIME_OFFSET] == PARTNER2) { //tripTime
        newInd.tripTime = p2.tripTime;
    }
    else if (mask[TRIPTIME_OFFSET] == AVG) {
        newInd.tripTime = p2.tripTime/2.0 + p1.tripTime/2.0;
    }
    if (mask[ZETA_OFFSET] == PARTNER2) { //zeta
        newInd.zeta = p2.zeta;
    }
    else if (mask[ZETA_OFFSET] == AVG) {
        newInd.zeta = p1.zeta/2.0 + p2.zeta/2.0;
    }
    if (mask[BETA_OFFSET] == PARTNER2) { //beta
        newInd.beta = p2.beta;
    }
    else if (mask[BETA_OFFSET] == AVG) {
        newInd.beta = p2.beta/2.0 + p1.beta/2.0;
    }
    if (mask[ALPHA_OFFSET] == PARTNER2) { //alpha
        newInd.alpha = p2.alpha;
    }
    else if (mask[ALPHA_OFFSET] == AVG) {
        newInd.alpha = p1.alpha/2.0 + p2.alpha/2.0;
    }

    return newInd;    
}

// utility function for mutate() to get a random double with as high a resolution as possible
// INPUT:
// max: the absolute value of the min and max(min = -max) of the range
double getRand(double max, std::mt19937_64 & rng) {
    return static_cast<double>(rng()) / rng.max() * max * 2.0 - max;
}


// in a given Individual's parameters, mutate one gene gauranteed. Randomly decide to mutate a second gene some times.
// mutate a gene by adding or subtracting a small, random value from a parameter
rkParameters<double> mutate(const rkParameters<double> & p1, mt19937_64 & rng, double annealing, cudaConstants* gConstant, thruster<double>& thrust) {
    rkParameters<double> newInd = p1;

    int genesToMutate = 1; // number of genes to mutate

    int mutateChance = rng() % 100;

    if (mutateChance < gConstant->triple_mutation_rate * 100) {
        genesToMutate = 3;
    }
    else if (mutateChance < gConstant->double_mutation_rate * 100) {
        genesToMutate = 2;
    }

    int mutatedGenes[3]; // index of genes to mutate

    mutatedGenes[0] = rng() % OPTIM_VARS;

    if (genesToMutate > 1) {
        do {
            mutatedGenes[1] = rng() % OPTIM_VARS;
        } while (mutatedGenes[1] == mutatedGenes[0]); // make sure that each mutated gene is unique
    }

    if (genesToMutate > 2) {
        do {
            mutatedGenes[2] = rng() % OPTIM_VARS;
        } while (mutatedGenes[2] == mutatedGenes[0] || mutatedGenes[2] == mutatedGenes[1]); // make sure that each mutated gene is unique
    }

    for (int i = 0; i < genesToMutate; i++) {
        int mutatedValue = mutatedGenes[i]; // the gene to mutate
        // alter thrust coefficients only when using a thruster
        if (thrust.type != thruster<double>::NO_THRUST) {
            //check coeff
            if ( (mutatedValue >= GAMMA_OFFSET) && (mutatedValue <= (GAMMA_OFFSET + GAMMA_ARRAY_SIZE-1)) ) {
                newInd.coeff.gamma[mutatedValue] += getRand(gConstant->gamma_mutate_scale * annealing, rng);
            }
            else if ( (mutatedValue >= TAU_OFFSET) && (mutatedValue <= (TAU_OFFSET+TAU_ARRAY_SIZE-1))) {//Tau (7-9)
                newInd.coeff.tau[mutatedValue-7] += getRand(gConstant->tau_mutate_scale * annealing, rng);
            }
            else if (mutatedValue >= COAST_OFFSET && mutatedValue <= (COAST_OFFSET + COAST_ARRAY_SIZE -1)) {//coast (14-18)
                newInd.coeff.coast[mutatedValue-14] += getRand(gConstant->coast_mutate_scale * annealing, rng);
            }
        }
        if (mutatedValue == TRIPTIME_OFFSET) { //Time final
            newInd.tripTime += SECONDS_IN_YEAR*getRand(gConstant->triptime_mutate_scale * annealing, rng);
        }
        if (mutatedValue == ZETA_OFFSET) { //zeta
            newInd.zeta += getRand(gConstant->zeta_mutate_scale * annealing, rng);
        }
        if (mutatedValue == BETA_OFFSET) { //beta
            newInd.beta += getRand(gConstant->beta_mutate_scale * annealing, rng);
        }
        if (mutatedValue == ALPHA_OFFSET) { //alpha
            newInd.alpha += getRand(gConstant->alpha_mutate_scale * annealing, rng);
        }
    }
    

    return newInd;    
}


// if using this, consider that theta is the same as theta + 2 pi

//Creates a new rkParameter based on the average between p1 and p2
// input: p1 and p2 are valid rkParameters
// output: average of the two
rkParameters<double> generateNewIndividual_avg(const rkParameters<double> & p1, const rkParameters<double> & p2, thruster<double>& thrust) {
    rkParameters<double> newInd;
    
    // alter thrust coefficients only when using a thruster
    if (thrust.type) {
        for (int i = 0; i < p1.coeff.gammaSize; i++) {
            //newInd.coeff.gamma[i] = (p1.coeff.gamma[i]/2.0) + (p2.coeff.gamma[i]/2.0);
        }
        for (int i = 0; i < p1.coeff.tauSize; i++) {
            //newInd.coeff.tau[i] = (p1.coeff.tau[i]/2.0) + (p2.coeff.tau[i]/2.0);
        }
        for (int i = 0; i < p1.coeff.coastSize; i++) {
            //newInd.coeff.coast[i] = (p1.coeff.coast[i]/2.0) + (p2.coeff.coast[i]/2.0);
        }
    }

    newInd.alpha = (p1.alpha/2.0) + (p2.alpha/2.0);
    newInd.beta = (p1.beta/2.0) + (p2.beta/2.0);
    newInd.zeta = (p1.zeta/2.0) + (p2.zeta/2.0);
    newInd.tripTime = (p1.tripTime/2.0) + (p2.tripTime/2.0);

    return newInd;    
}

// WIP
// Uses generateNewIndividual to create new Individual by crossing over properties with mask, followed by random chance for mutations
// Output is new individual in pool
// Can only be used within crossover function in its current state because of int i input
void mutateNewIndividual(Individual *pool, Individual *survivors, int mask[], int index, int i, double annealing, int poolSize, mt19937_64 & rng, cudaConstants* gConstant, thruster<double>& thrust) {
    pool[poolSize - 1 - (2 * index)] = Individual(); // create a new Individual instead of overwriting values
    pool[poolSize - 1 - (2 * index)].startParams = generateNewIndividual(survivors[2*i].startParams, survivors[(2*i)+1].startParams, mask, thrust);
    
    if (rng() % 100 < gConstant->mutation_rate * 100) { // a certain chance of mutation
        pool[poolSize - 1 - (2 * index)].startParams = mutate(pool[poolSize - 1 - (4 * index)].startParams, rng, annealing, gConstant, thrust);
    }

    flipMask(mask); // get the opposite offspring
    pool[poolSize - 1 - (2 * index) - 1] = Individual();
    pool[poolSize - 1 - (2 * index) - 1].startParams = generateNewIndividual(survivors[2*i].startParams, survivors[(2*i)+1].startParams, mask, thrust);
    
    if (rng() % 100 < gConstant->mutation_rate * 100) {
        pool[poolSize - 1 - (2 * index) - 1].startParams = mutate(pool[poolSize - 1 - (4 * index) - 1].startParams, rng, annealing, gConstant, thrust);
    }
}


int crossover(Individual *survivors, Individual *pool, int survivorSize, int poolSize, double annealing, cudaConstants* gConstant, thruster<double>& thrust) {
    mt19937_64 rng(gConstant->time_seed);

    int mask[OPTIM_VARS];

    int index = 0;

    // Generate two offspring through each crossover method, total of 8 offspring per parent pair
    for (int i = 0; i < survivorSize / 2; i++) {
        crossOver_wholeRandom(mask, rng);
        mutateNewIndividual(pool, survivors, mask, index, i, annealing, poolSize, rng, gConstant, thrust);
        index++;
    }

    for (int i = 0; i < survivorSize / 2; i++) {
        crossOver_average(mask);
        mutateNewIndividual(pool, survivors, mask, index, i, annealing, poolSize, rng, gConstant, thrust);
        index++;
    }

    for (int i = 0; i < survivorSize / 2; i++) {
        crossOver_bundleVars(mask, rng);
        mutateNewIndividual(pool, survivors, mask, index, i, annealing, poolSize, rng, gConstant, thrust);
        index++;
    }

    for(int i = 0; i < survivorSize / 2; i++){
        crossOver_bundleVars(mask, rng);
        mutateNewIndividual(pool, survivors, mask, index, i, annealing, poolSize, rng, gConstant, thrust);
        index++;
    }

    return index * 2;
}

//Unit Test for ga_crossover
/*
int main(){
    mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());
    
    // input parameters for rk4Simple which are the same for each thread
    coefficients<double> testcoeff;
    for(int i = 0; i < testcoeff.gammaSize; i++){
        testcoeff.gamma[i] = 1;
    }
    for(int i = 0; i < testcoeff.tauSize; i++){
        testcoeff.tau[i] = 1;
    }
    for(int i = 0; i < testcoeff.coastSize; i++){
        testcoeff.coast[i] = 1;
    }
    testcoeff.coastThreshold = 1;
    elements<double> spaceTest(1, 1, 1, 1, 1, 1);
    rkParameters<double> test1(1, 1, spaceTest, testcoeff); 

    // input parameters for rk4Simple which are the same for each thread
    coefficients<double> testcoeff2;
    for(int i = 0; i < testcoeff2.gammaSize; i++){
        testcoeff2.gamma[i] = 2;
    }
    for(int i = 0; i < testcoeff2.tauSize; i++){
        testcoeff2.tau[i] = 2;
    }
    for(int i = 0; i < testcoeff2.coastSize; i++){
        testcoeff2.coast[i] = 2;
    }
    testcoeff2.coastThreshold = 2;
    elements<double> spaceTest2(2, 2, 2, 2, 2, 2);
    rkParameters<double> test2(2, 2, spaceTest2, testcoeff2); 

    cout << "****** Test1 ******\n" << test1 << endl << endl;
    cout << "****** Test2 ******" << test2 << endl;

    //Set Up Mask
    int test_mask[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    int test_maskOther[OPTIM_VARS];
    cout << "               |  0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7" << endl;
    cout << "Starting Mask  : ";
    printMask(test_mask);
    cout << endl;

    //crossOver_randHalf(test_mask,rng);
    crossOver_wholeRandom(test_mask,rng);
    copyMask(test_mask,test_maskOther);
    flipMask(test_maskOther);

    cout << "First randHalf : ";
    printMask(test_mask);
    cout << endl;

    cout << "Second randHalf: ";
    printMask(test_maskOther);
    cout << endl;

    //Generating Offspring:
    rkParameters<double> output_1 = generateNewIndividual(test1,test2,test_mask);
    rkParameters<double> output_2 = generateNewIndividual(test1,test2,test_maskOther);
    rkParameters<double> output_3 = generateNewIndividual_avg(test1,test2);

    cout << "****** output_1 ******\n" << output_1 << endl << endl;
    cout << "****** output_2 ******\n" << output_2 << endl << endl;
    cout << "****** output_3 ******\n" << output_3 << endl << endl;

}
*/