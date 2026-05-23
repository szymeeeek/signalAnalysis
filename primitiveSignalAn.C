#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>


Bool_t saveSignal(std::string fileID = "20250217", std::string no = "21") {
    Bool_t debug = kFALSE;
    std::cout.precision(12);
    
    std::string filename = Form("%sscope_%s.csv", fileID.c_str(), no.c_str());
    std::fstream data;
    data.open(filename, ios::in);

    if (!data) {
        std::cout << "File couldn't be opened!" << std::endl;
        return kFALSE;
    }

    std::string paramsFilename = Form("%sscope_%s_PARAMS.txt", fileID.c_str(), no.c_str());
    std::fstream outParams;
    bool paramsExist = false;
    // Try to open for reading first
    outParams.open(paramsFilename, ios::in);
    if (outParams.good()) {
        paramsExist = true;
        std::cout<<"Reading analysis params from: "<<paramsFilename<<std::endl;
    } else {
        // If not exists, create for writing
        outParams.open(paramsFilename, std::ios::out);
        if(!outParams){
            std::cout<<"The params output file couldn't be opened!"<<std::endl;
            return kFALSE;
        }
        std::cout<<"Saving analysis params to: "<<paramsFilename<<std::endl;
    }

    TFile *signal = new TFile(Form("%s%ssignalSaved.root", fileID.c_str(), no.c_str()), "RECREATE");
    TTree *signTree = new TTree("signTree", "signTree");

    mySignal *ms = new mySignal();
    std::string branchName = Form("signal_%s", no.c_str());
    signTree->Branch(branchName.c_str(), &ms);

    Double_t BL, aThr;
    Double_t aMax, Q, t0, t1, t2, TOT; 

    //-------------------------------------------------------------------
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

    Double_t timeWindow; //in terms of bins

    Int_t nSamples;
    std::string dummy;
    data>>dummy>>dummy>>nSamples;
    std::cout<<"The number of samples in a single signal: "<<nSamples<<std::endl;

    std::string line;

    getline(data, line);
    getline(data, line);
    getline(data, line);

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

    TCanvas *preview = new TCanvas();
    preview->cd();
    signHisto->SetTitle("Preview of First Signal");
    signHisto->SetXTitle("time (ps)");
    signHisto->SetYTitle("voltage (V)");
    signHisto->SetLineColor(kBlue);
    signHisto->Draw();
    preview->Update();

    // Only ask for BL/aThr for preview if params file doesn't exist
    if (!paramsExist) {
        std::cout<<"baseline level: ";
        std::cin>>BL;

        std::cout<<"\namplitude threshold: ";
        std::cin>>aThr;

        std::cout<<"\ntime window: ";
        std::cin>>timeWindow;

        // Write BL and aThr to params file (once)
        outParams << BL << " " << aThr << std::endl;
        outParams.close();
    } else {
        // Read BL and aThr from params file
        Double_t blRead = 0, athrRead = 0;
        std::ifstream paramReader(paramsFilename);
        paramReader >> blRead >> athrRead;
        BL = blRead;
        aThr = athrRead;
        paramReader.close();

        std::cout<<"Read baseline level: "<<BL<<std::endl;
        std::cout<<"Read amplitude threshold: "<<aThr<<std::endl;

        std::cout<<"\ntime window: ";
        std::cin>>timeWindow;
    }

    preview->Close();
    delete preview;
    delete signHisto;

    std::cout<<"\n\n---------------------------------------"<<std::endl;
    std::cout<<"Processing signals from file: "<<filename<<std::endl;
    std::cout<<"Amplitude threshold: "<<aThr<<", time window: "<<timeWindow<<std::endl;
    std::cout<<"Baseline level: "<<BL<<std::endl;
    std::cout<<"----------------------------------------"<<std::endl;
    std::cout<<"Press enter to continue..."<<std::endl;
    std::cin.ignore();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    // Remove duplicate declaration of multiPreview
    // TCanvas *multiPreview = new TCanvas("multiPreview", "First 20 Signals", 1200, 800);
    // multiPreview->Divide(5,4);

    Int_t zeroChargeN = 0;
    int padDrawn = 0;
    int canvasIdx = 0;
    TCanvas *multiPreview = nullptr;

    // Add paramList declaration
    std::vector<std::tuple<Double_t, Double_t, Double_t>> paramList;

    // Read params if file exists
    if (paramsExist) {
        std::string paramLine;
        while (std::getline(outParams, paramLine)) {
            if (paramLine.empty()) continue;
            std::istringstream iss(paramLine);
            Double_t bl = 0, athr = 0, t0val = 0;
            if (!(iss >> bl >> athr >> t0val)) continue; // skip malformed lines
            paramList.push_back(std::make_tuple(bl, athr, t0val));
        }
        outParams.close();
    }

    while(data){
        // Create a new canvas for every batch of 20 signals
        if (padDrawn % 20 == 0) {
            if (multiPreview) {
                multiPreview->Write(Form("multiPreview_%d", canvasIdx));
                delete multiPreview;
                canvasIdx++;
            }
            multiPreview = new TCanvas(Form("multiPreview_%d", canvasIdx), Form("Signals %d-%d", padDrawn+1, padDrawn+20), 1200, 800);
            multiPreview->Divide(5,4);
        }

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

        // BL and aThr are now always constant for all signals, read from file or user
        // t0 is calculated per signal

        // Calculate t0 for this signal
        Int_t l = 1;
        for(Int_t i = (signHisto->GetNbinsX())/2-(signHisto->GetNbinsX())*0.1; i<=nSamples; i++){
            Double_t vAtI = signHisto->GetBinContent(i) - BL;
            if(vAtI > aThr){
                t0 = times.at(i-1);
                break;
            }
            l++;
        }

        // Subtract baseline from signal before drawing and analysis
        for(Int_t i = 0; i<nSamples; i++){
            signHisto->SetBinContent(i, signHisto->GetBinContent(i) - BL);
        }

        // Draw signal on the appropriate pad
        int padNum = (padDrawn % 20) + 1;
        multiPreview->cd(padNum);
        signHisto->SetTitle(Form("Signal %d", j));
        signHisto->SetXTitle("time (ps)");
        signHisto->SetYTitle("voltage (V)");
        signHisto->SetLineColor(kBlue);
        signHisto->Draw();

        // Amplitude threshold line
        TLine *athrln = new TLine(times.front(), aThr, times.back(), aThr);
        athrln->SetLineColor(kRed);
        athrln->SetLineStyle(2);
        athrln->SetLineWidth(2);
        athrln->Draw("same");

        // t0 line
        TLine *t0line = new TLine(t0, signHisto->GetMinimum(), t0, signHisto->GetMaximum());
        t0line->SetLineColor(kGreen+2);
        t0line->SetLineStyle(2);
        t0line->SetLineWidth(2);
        t0line->Draw("same");

        padDrawn++;

        //------------------------------------------------------------------------------
        // Calculating the parameters from the signals
        // 1. amplitude
        aMax = signHisto->GetMaximum();

        // 2. amplitude threshold - constant for the entire run.
        if(debug){std::cout<<"aThr = "<<aThr<<std::endl;}

        // 3. t0 & TOT
        Int_t z = 1;
        for(Int_t i = 1; i<=nSamples; i++){
            Double_t vAtI = signHisto->GetBinContent(i);
            if(vAtI>(aThr)){
                if(debug){cout<<i<<endl;}
                t0 = times.at(i-1);
                break;
            }
            z++;
        }

        //-------t1
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
        //////////////////////////////////////////////////////////////////////////////////////

        ms->set(t0, TOT, aMax, Q);
        if(debug){signHisto->Write();}

        signTree->Fill();
        j++;
    }

    // Write and clean up the last canvas if it exists
    if (multiPreview) {
        multiPreview->Write(Form("multiPreview_%d", canvasIdx));
        delete multiPreview;
    }

    std::cout<<"\n\n---------------------------------------"<<std::endl;
    std::cout<<"Number of histos with charge under 1 C: "<<zeroChargeN<<std::endl;
    signTree->Write();
    signal->Close();
    if(outParams.is_open()) outParams.close();
    
    return kTRUE;
}