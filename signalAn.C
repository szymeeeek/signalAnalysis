#include <iostream>
#include <fstream>
#include <string>

/*The first two arguments are for specifying the filename, its format should be "[fileID]_scope_[no].csv".*/

Bool_t saveSignal(std::string fileID = "20250217", std::string no = "21"){
    gSystem->Load("mySignal_cxx.so");

    std::string filename = Form("%s_scope_%s.csv", fileID.c_str(), no.c_str());
    std::ifstream data;
    data.open(filename, ios::in);

    if(!data){
        std::cout<<"File coudn't be opened!"<<std::endl;
        return kFALSE;
    }

    TFile *signal = new TFile(Form("%ssignalSaved.root", fileID.c_str()), "UPDATE");
    TTree *signTree = new TTree("signTree", "signTree");

    mySignal *ms = new mySignal();
    std::string branchName = Form("%s_%s", fileID.c_str(), no.c_str());
    signTree->Branch(branchName.c_str(), &ms);

    Double_t aMax, aThr, t0, t1, Q, TOT;
    Int_t baselineCount = 100;

    Double_t time, voltage;
    int j = 0;

    Int_t nSamples;
    std::string dummy;
    data>>dummy>>dummy>>nSamples;
    std::cout<<nSamples<<std::endl;

    data.ignore(10000, '\n');
    data.ignore(10000, '\n');

    while(!data){
        TH2D *signHisto = new TH2D(Form("histo%i", j), Form("histo%i", j), nSamples, 0, nSamples, 0, 10);
        std::vector <Double_t> baseline;

        for(Int_t i = 0; i<nSamples; i++){
            data>>time>>voltage;
            signHisto->SetBinContent(time, voltage);
            if(i<baselineCount){
                baseline.push_back(voltage);
            }
        }

        aMax = signHisto->GetMaximum();

        Double_t sum = 0;
        for (Double_t v : baseline) {
            sum += v;
        }
        Double_t mean = sum / baseline.size();
        
        Double_t variance = 0;
        for (Double_t v : baseline) {
            variance += (v - mean) * (v - mean);
        }
        variance /= baseline.size();
        Double_t stddev = std::sqrt(variance);
        aThr = 3*stddev;

        for(Int_t i = 0; i<nSamples; i++){
            t0 = signHisto->GetBinContent(i);
            if(t0>aThr){
                for(Int_t k = i; k<nSamples; k++){
                    t1 = signHisto->GetBinContent(k);
                    if(t1<aThr){
                        break;
                    }
                }
            }
        }

        TOT = t1 - t0;
        Q = signHisto->Integral(t0, t1, 0, aMax);

        std::cout<<"Histo no. "<<j+1<<std::endl;

        ms->set(t0, TOT, aMax, Q);
        signTree->Fill();
        signHisto->Write();
        signHisto->Delete();
        j++;
    }
    signTree->Write();
    signal->Close();

    return kTRUE;
}

Bool_t histosMaking(std::string rootFile = "20250213"){
    std::string filename = Form("%ssignalSaved", rootFile.c_str());
    TFile *infile = new TFile(filename.c_str(), "READ");

    if(!infile){
        std::cout<<"Something's wrong! File couldn't be opened!"<<std::endl;
    }

    return kTRUE;
}