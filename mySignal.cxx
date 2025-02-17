#include "mySignal.h"
#include <iostream>

mySignal::mySignal() : t0(0), TOT(0), aMax(0), Q(0) {
    std::cout << "Creating...  " << std::endl;
    Print();
}

mySignal::mySignal(Long_t t, Long_t T, Double_t A, Double_t Q1) : t0(t), TOT(T), aMax(A), Q(Q1){
    std::cout << "Creating... "<< std::endl;
    Print();
}

mySignal::~mySignal(){
    std::cout << "Destroying the object." << std::endl;
}

void mySignal::Print(Option_t *option) const {
    std::cout << "t0 = " << t0 << "; TOT = " << TOT << "; aMax = " << aMax << "; Q = " << Q << std::endl;
}