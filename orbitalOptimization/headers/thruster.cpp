#include "thruster.h"
#include <iomanip> // setprecision(int)


// sets starting values as given
template <class T>
thruster<T>::thruster(int newType){
    if(newType == 1){
        m_Dot = 5.73E-06;
        P0 = NEXTP0;
    }
type = newType;
}

template <class T> T thruster<T>::calc_eff(const T & Pin){
   if(type == 1){
    return  -1.328086e-23*pow(Pin,6) + 6.207694e-19*pow(Pin,5) - 9.991813e-15*pow(Pin,4) +  7.701266e-11*pow(Pin,3) - 3.136031e-07*pow(Pin,2) +  6.805225e-04*Pin;
   }
}

template <class T> void thruster<T>::calc_m_Dot(const T & Pin){
    if (type == 1){
        if(Pin < 2550){
            this->m_Dot = 1.99E-06;
        }
        else if(Pin < 4500){
            this->m_Dot = 4.44E-06;
        }
        else {
            this->m_Dot = 5.73E-06;
        }

    }
}


template <class T> std::ostream & operator<<(std::ostream & Str, const thruster<T> & e) {
    Str << std::fixed;
    Str << std::setprecision(16); // number of decimals output into text file
    Str << e.gamma << "\t" << e.tau << "\n";
    return Str;
}