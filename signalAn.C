#include "mySignal.h"
#include <iostream>
#include <ifstream>

Bool_t saveSignal(TString fileID = "20250213", Int_t no = "0"){
    //reading different files than .csv ones requires changing the line below
    TString filename = Form("%s_scope_%i.csv", fileID, no);
    std::ifstream data;
    data.open(filename, ios::in);

    if(!data){
        std::cout<<"File coudn't be opened!"<<std::endl;
        return kFALSE;
    }

    TFile *signal = new TFile(Form("%ssignalSaved", fileID), "UPDATE");
    TTree *signTree = new TTree("signTree", "signTree");

    mySignal *ms = new mySignal();
    TString branchName = Form("%s_%i", fileID, no);
    signTree->Branch(branchName, &ms);

    /*tu dodac deklaracje zmiennych*/

    data.ignore(10000, '\n');
    while(data>>/**/){
        ms->set();
        signTree->Fill();
    }
    signTree->Write();
    signal->Close();

    return kTRUE;
}

Bool_t histosMaking(TString rootFile = "20250213"){
    TString filename = Form("%ssignalSaved", rootFile);
    TFile *infile = new TFile(filename, "READ");

    if(!infile){
        std::cout<<"Something's wrong! File couldn't be opened!";
    }

    return kTRUE;
}