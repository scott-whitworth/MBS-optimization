#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include "config.h"

// Constructors uses geneticFileRead() to set the struct's properties from a default config file located in same folder as executable
geneticConstants::geneticConstants() {
    geneticFileRead("genetic.config");
}
// Operates same as default, however uses configFile as address for where the config file to be used is located
geneticConstants::geneticConstants(std::string configFile) {
    geneticFileRead(configFile);
}


//http://www.cplusplus.com/forum/beginner/11304/ for refesher on reading line by line
void geneticConstants::geneticFileRead(std::string fileName) {

    std::string line;
    std::ifstream configFile;
    configFile.open(fileName);
    if (configFile.is_open()) {
        // Go through line by line
        while ( std::getline(configFile, line ) ) {
            // If line is not empty and the line is not a comment, then the line should be a variable constant being assigned 
            if (line != "" && ( line.find_first_of("//") == std::string::npos ) && (line.find_first_of(" ") == std::string::npos) ) {
                // Go through if statements to find what constant variable this line refers to (look at substring prior to "=") and attempt to assign the value (after "=") to the variable
                std::string variableName = line.substr(0, line.find("=")   );
                std::string variableValue = line.substr( line.find("=") + 1); // Currently reads variableValue to end of the line string, may want to implement an end position to allow comments appends in the file format

                // Assign variableValue to the appropriate variable based on variableName, with proper conversion to the right data type
                // Still need to add a check to ensure that variableValue is converted properly (if not valid, don't assign property to variableValue and note that in the terminal)
                if (variableName == "pos_threshold") {
                    this->pos_threshold = std::stod(variableValue);
                }
                else if (variableName == "speed_threshold") {
                    this->speed_threshold = std::stod(variableValue);
                }
                else if (variableName == "thruster_type") {
                    this->thruster_type = std::stoi(variableValue);
                }
                else if (variableName == "anneal_factor") {
                    this->anneal_factor = std::stod(variableValue);
                }
                else if (variableName == "write_freq") {
                    this->write_freq = std::stoi(variableValue);
                }
                else if (variableName == "disp_freq") {
                    this->disp_freq = std::stoi(variableValue);
                }
                else if (variableName == "best_count") {
                    this->best_count = std::stoi(variableValue);
                }
                else if (variableName == "change_check") {
                    this->change_check = std::stoi(variableValue);
                }
                else if (variableName == "anneal_initial") {
                    this->anneal_initial = std::stod(variableValue);
                }
                else if (variableName == "mutation_rate") {
                    this->mutation_rate = std::stod(variableValue);
                }
                else if (variableName == "double_mutate_rate") {
                    this->double_mutation_rate = std::stod(variableValue);
                }
                else if (variableName == "triple_mutate_rate") {
                    this->triple_mutation_rate = std::stod(variableValue);
                }
                else if (variableName == "gamma_mutate_scale") {
                    this->gamma_mutate_scale = std::stod(variableValue);
                }
                else if (variableName == "tau_mutate_scale") {
                    this->tau_mutate_scale = std::stod(variableValue);
                }
                else if (variableName == "coast_mutate_scale") {
                    this->coast_mutate_scale = std::stod(variableValue);
                }
                else if (variableName == "triptime_mutate_scale") {
                    this->triple_mutation_rate = std::stod(variableValue);
                }
                else if (variableName == "zeta_mutate_scale") {
                    this->zeta_mutate_scale = std::stod(variableValue);
                }
                else if (variableName == "beta_mutate_scale") {
                    this->beta_mutate_scale = std::stod(variableValue);
                }
                else if (variableName == "alpha_mutate_scale") {
                    this->alpha_mutate_scale = std::stod(variableValue);
                }
                else if (variableName == "time_seed" && variableValue != "NONE") { // If the conifguration sets time_seed to NONE then time_seed is set to (time(0)) 
                    this->time_seed = std::stod(variableValue);
                }
                else if (variableName == "time_seed" && variableValue == "NONE") {
                    this->time_seed = time(0);
                    std::cout << "time_seed set to time(0)\n";
                }
                else {
                    std::cout << "Unknown variable '" << variableName <<"' in genetic.config!\n";
                }
            }
        }
    }
    else {
        std::cout << "Unable to open config file!\n";
    }
}
