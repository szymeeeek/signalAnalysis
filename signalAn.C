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

    TFile *signal = new TFile(Form("%s_%ssignalSaved.root", fileID.c_str(), no.c_str()), "UPDATE");
    TTree *signTree = new TTree("signTree", "signTree");

    mySignal *ms = new mySignal();
    std::string branchName = Form("%s_%s", fileID.c_str(), no.c_str());
    signTree->Branch(branchName.c_str(), &ms);

    Double_t aMax, aThr, Q;
    Long_t t0, t1, TOT;
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

        //////////////////////////////////////////////////////////////////////////////////////
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
        //////////////////////////////////////////////////////////////////////////////////////

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

Bool_t histosMaking(std::string rootFile = "20250217", std::string no = "21"){
    gStyle->SetOptStat(0);
    
    //opening a file containing the tree and reading it
    std::string filename = Form("%s_%ssignalSaved.root", rootFile.c_str(), no.c_str());
    TFile *infile = new TFile(filename.c_str(), "READ");
    if(!infile){
        std::cout<<"Something's wrong! File couldn't be opened!"<<std::endl;
        return kFALSE;
    }

    std::string branchName = Form("%s_%s", rootFile.c_str(), no.c_str());
    TTree *tree = (TTree*)infile->Get("signTree");
    if (!tree) {
        std::cout << "Tree not found!" << std::endl;
        return kFALSE;
    }

    Double_t aMax, Q;
    Long_t t0, TOT;
    Int_t nEntries = tree->GetEntries();
    tree->SetBranchAddress("t0", &t0);
    tree->SetBranchAddress("TOT", &TOT);
    tree->SetBranchAddress("aMax", &aMax);
    tree->SetBranchAddress("Q", &Q);

    //getting the ranges
    Double_t Qlow, Qup, aMaxLow, aMaxUp, TOTlow, TOTup, t0low, t0up;
    tree->GetEntry(0);
    Qlow = 0.9*Q; aMaxLow = 0.9*aMax; TOTlow = 0.9*TOT; t0low = 0.9*t0;
    tree->GetEntry((tree->GetEntries())-1);
    Qup = 1.1*Q; aMaxUp = 1.1*aMax; TOTup = 1.1*TOT; t0up = 1.1*t0;
    
    //creating histos
    TH1D *Qh = new TH1D("Qh", "Q", 100, Qlow, Qup); //
    TH1D *aMaxh = new TH1D("aMaxh", "aMax", 20, aMaxLow, aMaxUp);
    TH1I *TOTh = new TH1I("TOTh", "TOT", 20, TOTlow, TOTup);
    TH1I *t0h = new TH1I("t0h", "t0", 100, t0low, t0up);
    TH2D *aMaxQh = new TH2D("aMaxQh", "aMax vs. Q", 50, aMaxLow, aMaxUp, 100, Qlow, Qup);
    TH2D *Qt0h = new TH2D("Qt0h", "Q vs. t0", 100, Qlow, Qup, 100, t0low, t0up);

    //filling the histos
    for(Int_t i = 0; i < nEntries; i++){
        tree->GetEntry(i);

        t0h->Fill(t0);
        TOTh->Fill(TOT);
        aMaxh->Fill(aMax);
        Qh->Fill(Q);

        aMaxQh->Fill(aMax, Q);
        Qt0h->Fill(Q, t0);
    }

    //fitting
    TF1 *aGaus = new TF1("aGaus", "gaus", aMaxLow, aMaxUp);
    TF1 *QGaus = new TF1("QGaus", "gaus", Qlow, Qup);

    TFitResultPtr aRes = aMaxh->Fit(aGaus, "S");
    TFitResultPtr QRes = Qh->Fit(QGaus, "S");

    //drawing and saving the histos
    std::string outFilename = Form("%s_scope_%s_HISTOS.root", rootFile.c_str(), no.c_str());
    TFile *outfile = new TFile(outFilename.c_str(), "RECREATE");

    std::string aParStr = Form("mean = %f +/- %f", aGaus->GetParameter(1), aGaus->GetParError(1));
    TText *aMaxPars = new TText();
    //std::cout<<"aPars "<<aParStr<<std::endl;

    std::string QParStr = Form("mean = %f +/- %f", QGaus->GetParameter(1), QGaus->GetParError(1));
    TText *QPars = new TText();
    //std::cout<<"QPars "<<QParStr<<std::endl;

    TCanvas *c1 = new TCanvas();
    c1->Divide(2, 2);

    c1->cd(1);
    gStyle->SetOptStat(1);
    t0h->SetTitle("t0; t0 (ps); counts (a.u.)");
    t0h->Draw();
    t0h->Write();

    c1->cd(2);
    gStyle->SetOptStat(1);
    TOTh->SetTitle("TOT; TOT (ps); counts (a.u.)");
    TOTh->Draw();
    TOTh->Write();

    c1->cd(3);
    gStyle->SetOptStat(1);
    Qh->SetTitle("Q; Q (C); counts (a.u.)");
    Qh->Draw();
    QPars->DrawTextNDC(.5, .8, QParStr.c_str());
    Qh->Write();

    c1->cd(4);
    gStyle->SetOptStat(1);
    aMaxh->SetTitle("aMax; aMax (V); counts (a.u.)");
    aMaxh->Draw();
    aMaxPars->DrawTextNDC(0.5, 0.8, aParStr.c_str());
    aMaxh->Write();

    c1->Write();


    TCanvas *c2 = new TCanvas();
    c2->Divide(2, 1);

    c2->cd(1);
    gStyle->SetOptStat(1);
    aMaxQh->SetTitle("aMax vs. Q; aMax (V); Q (C)");
    aMaxQh->Draw();
    aMaxQh->Write();

    c2->cd(2);
    gStyle->SetOptStat(1);
    Qt0h->SetTitle("Q vs. t0; Q (C); t0 (ps)");
    Qt0h->Draw();
    Qt0h->Write();

    c2->Write();

    return kTRUE;
}