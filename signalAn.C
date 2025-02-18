#include <iostream>
#include <fstream>
#include <string>

/*The first two arguments are for specifying the filename, its format should be "[fileID]_scope_[no].csv".*/

Bool_t saveSignal(std::string fileID = "20250217", std::string no = "21"){
    Bool_t debug = kFALSE;
    gSystem->Load("mySignal_cxx.so");
    std::cout.precision(12);

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
    Int_t t0i, t1i;
    Int_t baselineCount = 100;

    double time, voltage;
    int j = 0;

    Int_t nSamples;
    std::string dummy;
    data>>dummy>>dummy>>nSamples;
    std::cout<<"The number of samples in a single signal: "<<nSamples<<std::endl;

    std::string line;

    getline(data, line);
    getline(data, line);
    getline(data, line);

    while(data){
        TH1D *signHisto = new TH1D(Form("histo%i", j), Form("histo%i", j), nSamples, 0, nSamples);
        std::vector <Double_t> baseline = {};
        std::vector <Double_t> times;

        for(Int_t i = 0; i<nSamples; i++){
            std::getline(data, line, ',');
            time = line.empty() ? 0 : stod(line);
            std::getline(data, line, '\n');
            voltage = line.empty() ? 0 : stod(line);

            signHisto->SetBinContent(i, voltage);
            if(i<baselineCount){
                baseline.push_back(voltage);
            }
            times.push_back(time);
        }
        signHisto->SetBins(nSamples, times.at(0), times.at(times.size()-1));

        /*Calculating the parameters from the signals*/
        //1. amplitude
        aMax = signHisto->GetMaximum();

        //2. amplitude threshold
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
        aThr = 5*stddev;
        if(debug == true){std::cout<<aThr<<std::endl;}

        //3. t0&TOT
        Int_t l = 1;
        for(Int_t i = 1; i<=nSamples; i++){
            Double_t vAtI = signHisto->GetBinContent(i);
            if(vAtI>aThr){
                if(debug == true){cout<<i<<endl;}
                t0 = times.at(i-1);
                break;
            }
            l++;
        }
        for(Int_t k = l+10; k<=nSamples; k++){
            Double_t vAtI = signHisto->GetBinContent(k);
            if(vAtI<aThr){
                t1 = times.at(k-1);
                if(debug == true){cout<<k<<endl;}
                break;
            }
        }
        std::setprecision(10);

        if(debug == true){cout<<t1<<" "<<t0<<endl;}
        TOT = t1 - t0;

        // 4. Charge
        Int_t binT0 = signHisto->FindBin(t0);
        Int_t binT1 = signHisto->FindBin(t1);

        Q = signHisto->Integral(binT0, binT1);

        ms->set(t0, TOT, aMax, Q);
        signTree->Fill();
        if(debug == true){signHisto->Write();}
        delete signHisto;
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