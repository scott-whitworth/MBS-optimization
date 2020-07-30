#ifndef GA_CROSSOVER_h
#define GA_CROSSOVER_h

#include <random>

#include "../Runge_Kutta/rkParameters.h"
#include "individuals.h"
#include "../Thrust_Files/thruster.h"
#include "../Config_Constants/config.h"

// Method of determing selection of survivors that will carry properties into the new individuals of the newGeneration
// Input: pool - a shuffled pointer array of individuals to choose from
//        selectionSize - integer number of how many survivors to choose out of the pool
//        survivors - pointer array of individuals to copy the selected individuals and store
//        cConstants - for accessing values such as number of individuals
// Output: pool is unchanged, survivors contains an array of size selectionSize of individuals that were quasi-randomly chosen
void selectSurvivors(Individual * pool, int selectionSize, Individual* survivors, const cudaConstants * cConstants);

// Creates a random bifurcation mask, currently not in use
// Randomly picks one index to be the start of the '2's from mask
void crossOver_randHalf(int * mask, std::mt19937_64 & rng);

// Creates a mask that is contains randomly chosen values in each index
// Each element in a mask is randomly set to either PARTNER1 or PARTNER2
void crossOver_wholeRandom(int * mask, std::mt19937_64 & rng);

// This crossover method randomly chooses between partners for each variable. Similar to crossOver_wholeRandom, but this keeps all parameters types (gamma, tau, coast) grouped
// Input: int pointer array mask of size OPTIM_VARS, rng that is a constructed mt19937_64 random number generator used to derive random values between 1 and 2
// Output: mask will contain an array of values that is either 1 or 2 (equivalent to PARTNER1 or PARTNER2)
void crossOver_bundleVars(int * mask, std::mt19937_64 & rng);

// Sets the entire mask to be AVG for length OPTIM_VARS
// Input: mask - pointer integer array of length OPTIM_VARS
// Output: mask is set to contain all AVG values
void crossOver_average(int * mask);

// Utility to flip the polarity of a mask
// Input:  mask is an array of size OPTIM_VARS, input based on maskValue enumerations as a mask
// Output: each PARTNER1 in mask will be reassigned to be a PARTNER2, each PARTNER2 will be reassigned PARTNER1, AVG will be unchanged
void flipMask(int * mask);

// Copy contents of maskIn into maskOut of size OPTIM_VARS
// Input: maskIn an array that is the original mask, maskOut is an array that will have contents copied to
// Output: maskIn remains unchanged, maskOut will have same contents as maskIn
void copyMask(int * maskIn, int * maskOut);

// Utility function for mutate() to get a random double with high resolution
// Input: max - the absolute value of the min and max(min = -max) of the range
// Output: A double value that is between -max and +max
double getRand(double max, std::mt19937_64 & rng);

// Creates a new rkParameters individual by combining properties of two parent Individuals using a mask to determine which
// Input: two rkParameter individuals (p1 and p2) - used in creating the new individual
//        mask - Contains maskValue values and length of OPTIM_VARS, determines how the two parent properties are merged into creating the new individual
//        thrust - Determine if the thruster properties must be carried over
//        cConstants - passed to mutate()
//        annealing - passed to mutate()
//        rng - passed to mutate()
//        generation - passed to mutate()
// Output: Returns rkParameter object that is new individual
rkParameters<double> generateNewIndividual(const rkParameters<double> & p1, const rkParameters<double> & p2, const int * mask, const cudaConstants * cConstants, double annealing, std::mt19937_64 & rng, double generation);

// Utility function to generate a boolean mask that determines which parameter value is mutating and how many based on mutation_rate iteratively
// input: rng - random number generating object used to randomly generate index values
//        mutateMask - pointer to a boolean array that is assumed length of OPTIM_VARS, sets genes being mutated to true and others to false
//        mutation_rate - a double value  less than 1 that is the chance a gene will be mutated, called iteratively to mutate more genes
// output: mutateMask contains false for genes that are not mutating, true for genes that are to be mutated
void mutateMask(std::mt19937_64 & rng, bool * mutateMask, double mutation_rate);

// In a given Individual's parameters, generate a mutate mask using mutateMask() and then adjust parameters based on the mask, mutation of at least one gene is not guranteed
// mutate a gene by adding or subtracting a small, random value on a parameter property
// Input: p1 - rkParameter that is taken to be the mutation base
//        rng - random number generator to use
//        annealing - a scalar value on the max random number when mutating
//        cConstants - holds properties to use such as mutation rates and mutation scales for specific parameter property types
//        thrust - Used to check if the thruster needs to be taken into account
// Output: Returns rkParameter object that is the mutated version of p1
rkParameters<double> mutate(const rkParameters<double> & p1, std::mt19937_64 & rng, double annealing, const cudaConstants* gConstant, double generation);

// Method that creates a pair of new Individuals from a pair of other individuals and a mask
// Input: pool - pointer array to Individuals that is where the new pair of individuals are stored
//        survivors - pointer array to Individuals to access the two parents from
//        mask - pointer array of maskValues used to decide on which property from which parent is acquired (or average of the two)
//        newIndCount - value that tracks number of newly created indiviudals in the pool so far in the newGeneration process, also impacts where to put the new individuals in the pool
//        int parentsIndex - value for determing where the pair of parent survivors are selected (parent 1 is at parentsIndex, parent 2 is offset by +1)
//        annealing - double variable passed onto mutateNewIndividual
//        poolSize - length of the pool array
//        rng - random number generator passed on to mutateNewIndividual
//        cConstants - passed on to mutateNewIndividual
// Output: pool contains two newly created individuals at (poolSize - 1 - newIndCount) and (poolSize - 2 - newIndCount)
//         mask is flipped in polarity (refer to flipMask method)
//         newIndCount is incremented by +2
void generateChildrenPair(Individual *pool, Individual *survivors, int * mask, int& newIndCount, int parentsIndex, double annealing, int poolSize, std::mt19937_64 & rng, const cudaConstants* cConstants, double generation);

// Creates the next pool to be used in the optimize function in opimization.cu
// Input: survivors - Individual pointer array of Individuals to be used in creating new individuals
//        pool - Sorted Individual pointer array that contains current generation
//        survivorSize - length of survivors array
//        poolSize - length of pool array
//        annealing - passed onto generateChildrenPair
//        rng - Random number generator to use for making random number values
//        cConstants - passed onto generateChildrenPair
//        thrust - passed onto generateChildrenPair
// Output: lower (survivorSize * 4) portion of pool is replaced with new individuals
//         Returns number of new individuals created (newIndCount)
int newGeneration(Individual *survivors, Individual *pool, int survivorSize, int poolSize, double annealing, const cudaConstants* cConstants, std::mt19937_64 & rng, double generation );

#include "ga_crossover.cpp"
#endif

