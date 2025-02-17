#include "mySignal.h"
#include <iostream>

mySignal::mySignal() : t0(0), V(0), TOT(0), aMax(0), Q(0) {
    std::cout << "Creating...  " << std::endl;
    Print();
}

mySignal::mySignal(Long_t t, Double_t volt, Long_t T, Double_t A, Double_t Q1) : t0(t), V(volt), TOT(T), aMax(A), Q(Q1){
    std::cout << "Creating... "<< std::endl;
    Print();
}

mySignal::~mySignal(){
    std::cout << "Destroying the object." << std::endl;
}

void mySignal::Print(){
    std::cout << "t0 = " << t0 << "; V = " << V << "; TOT = " << TOT << "; aMax = " << aMax << "; Q = " << Q << std::endl;
}