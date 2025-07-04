#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>


Bool_t saveSignal(std::string fileID = "20250217", std::string no = "21") {
    Bool_t debug = kTRUE;
    gSystem->Load("mySignal_cxx.so");
    std::cout.precision(12);
    
    std::string filename = Form("%sscope_%s.csv", fileID.c_str(), no.c_str());
    std::fstream data;
    data.open(filename, ios::in);

    if (!data) {
        std::cout << "File couldn't be opened!" << std::endl;
        return kFALSE;
    }

    TFile *signal = new TFile(Form("%s%ssignalSaved.root", fileID.c_str(), no.c_str()), "RECREATE");
    TTree *signTree = new TTree("signTree", "signTree");

    mySignal *ms = new mySignal();
    std::string branchName = Form("signal_%s", no.c_str());
    signTree->Branch(branchName.c_str(), &ms);

    Double_t aMax, aThr, Q, t0, t1, t2, TOT; 

    // ... (rest of the //-------------------------------------------------------------------
    //  t0 is the start time of the signal, common for Q and TOT calculation, 
    //  t1 is the upper boundary for Q calculation (constant time window),
    //  t2 is the upper boundary for TOT calculated dynamically
    //-------------------------------------------------------------------

    Int_t baselineCount = 80;

    //------------------------------------------------------------------------------
    //  baselineCount is the number of bins used to calculate the mean baseline value
    //------------------------------------------------------------------------------

    double time, voltage;
    int j = 0;

    Double_t timeWindow = 275; //in terms of bins

    Int_t nSamples;
    std::string dummy;
    data>>dummy>>dummy>>nSamples;
    std::cout<<"The number of samples in a single signal: "<<nSamples<<std::endl;

    std::string line;

    getline(data, line);
    std::cout<<line<<std::endl;
    getline(data, line);
    std::cout<<line<<std::endl;
    getline(data, line);
    std::cout<<line<<std::endl;
    
    Int_t zeroChargeN = 0;

    while(data){
        TH1D *signHisto = new TH1D(Form("histo%i", j), Form("histo%i", j), nSamples, 0, nSamples);
        signHisto->SetDirectory(nullptr);
        std::vector <Double_t> baseline = {};
        std::vector <Double_t> times;

        for(Int_t i = 0; i<nSamples; i++){
            std::getline(data, line, ',');
            time = line.empty() ? 0 : stod(line)*1e12;
            std::getline(data, line, '\n');
            voltage = line.empty() ? 0 : stod(line);

            signHisto->SetBinContent(i, voltage);
            if(i<baselineCount){
                baseline.push_back(voltage);
            }
            times.push_back(time);
        }
        signHisto->SetBins(nSamples, times.at(0), times.at(times.size()-1));

    //------------------------------------------------------------------------------

    /*Calculating the parameters from the signals*/
        
    //1. amplitude
        aMax = signHisto->GetMaximum();

    //2. amplitude threshold (bsln & variance not used atm)
        
        //-------baseline
        Double_t sum = 0;
        for (Double_t v : baseline) {
            sum += v;
        }
        Double_t mean = sum / baseline.size();
        
        //-------variance & standrad deviation
        Double_t variance = 0;
        for (Double_t v : baseline) {
            variance += (v - mean) * (v - mean);
        }
        variance /= baseline.size();
        Double_t stddev = std::sqrt(variance);

        //-------amplitude threshold - constant for the entire run.
        aThr = 0.001;
        if(debug){std::cout<<"aThr = "<<aThr<<std::endl;}

    //3. t0&TOT
        //-------t0
        Int_t l = 1;
        for(Int_t i = 1; i<=nSamples; i++){
            Double_t vAtI = signHisto->GetBinContent(i);
            if(vAtI>(aThr)){
                if(debug){cout<<i<<endl;}
                t0 = times.at(i-1);
                break;
            }
            l++;
        }

        //std::cout<<"\n\n l = "<<l<<", thus l+275 = "<<l+275<<"\n\n"<<"iter no. "<<j<<"\n\n"<<std::endl;

        //---------t1
        try{
            t1 = times.at(l+timeWindow);
        }
        catch(const std::out_of_range& e){
            std::cout<<"exception caught: "<<e.what()<<std::endl;
            continue;
        }
        
        //tutaj przekroczenie dla tot zrobic
        Int_t k = l+10;
        for( ; k<=nSamples; k++){
            Double_t vAtI = signHisto->GetBinContent(k);
            if(vAtI<aThr){
                t2 = times.at(k-1);
                if(debug){cout<<k<<endl;}
                break;
            }
        }
        
        std::setprecision(10);

        //-------TOT
        if(debug == true){cout<<t1<<" "<<t0<<endl;}
        TOT = t2 - t0;

    // 4. Charge
        Int_t binT0 = signHisto->FindBin(t0);
        Int_t binT1 = signHisto->FindBin(t1);

        Q = signHisto->Integral(binT0, binT1);
        //if(Q < 1e-10){std::cout<<"Q = "<<Q<<"; iteration no. "<<j<<std::endl;}
        //////////////////////////////////////////////////////////////////////////////////////

        TLine *bsln = new TLine(times.front(), aThr, times.back(), aThr);
        bsln->SetLineColor(kRed);
        bsln->SetLineStyle(2);
        bsln->SetLineWidth(2);

        TLine *startline = new TLine(t0, signHisto->GetMinimum(), t0, signHisto->GetMaximum());
        startline->SetLineColor(kRed);
        startline->SetLineStyle(2);
        startline->SetLineWidth(2);

        // Add TText to describe the lines
        TText *bslnText = new TText(times.front(), aThr + 0.02 * (signHisto->GetMaximum() - signHisto->GetMinimum()), "Amplitude threshold");
        bslnText->SetTextColor(kRed);
        bslnText->SetTextSize(0.03);

        TText *startlineText = new TText(t0 + 0.02 * (times.back() - times.front()), signHisto->GetMaximum(), "t0 (Start time)");
        startlineText->SetTextColor(kRed);
        startlineText->SetTextSize(0.03);

        TCanvas *c1 = new TCanvas();
        c1->cd();
        signHisto->SetTitle("");
        signHisto->SetXTitle("time (ps)");
        signHisto->SetYTitle("voltage (V)");
        signHisto->SetLineColor(kBlue);
        signHisto->SetLineWidth(2);
        signHisto->Draw();
        bsln->Draw("same");
        startline->Draw("same");
        startlineText->Draw("same");
        bslnText->Draw("same");

        c1->Write();

        ms->set(t0, TOT, aMax, Q);
        if(debug){signHisto->Write();}

        if(TMath::Abs(Q)<0.2){
            signHisto->Write();
            std::cout<<"Q < 1!!: "<<Q<<", iter no. "<<j<<std::endl;
            std::cout<<"aThr = "<<aThr<<", t0 = "<<t0<<", bin no. "<<l<<", t1 = "<<t1<<", bin no. "<<l+timeWindow<<std::endl;
            zeroChargeN++;
            j++;
            delete signHisto;
            continue;
        }

        signTree->Fill();
        delete signHisto;
        j++;
    }
    std::cout<<"\n\n---------------------------------------"<<std::endl;
    std::cout<<"Number of histos with charge under 1 C: "<<zeroChargeN<<std::endl;
    signTree->Write();
    signal->Close();code remains unchanged)
    
    return kTRUE;
}