#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

//come up with a filenaming method
//get filenames using the firectory function from (...)
//sort them alphabetically
//specify the number of files for one position

//add the dynamic version of calculating the charge and TOT

std::vector<std::pair<std::string, int>> dir_lister(std::string path_str) {
    std::vector<std::pair<std::string, int>> files;
    std::filesystem::path path(path_str);

    if (!std::filesystem::exists(path)) {
        std::cerr << "Path does not exist." << std::endl;
        return files;
    }
    if (!std::filesystem::is_directory(path)) {
        std::cerr << "Path is not a directory." << std::endl;
        return files;
    }
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_regular_file(entry.symlink_status())) {
            std::string file_name = entry.path().filename().string();
            int file_size = static_cast<int>(entry.file_size());
            files.emplace_back(file_name, file_size);
            // std::cout << "File: " << file_name << ", Size: " << file_size << " bytes" << std::endl;
        }
    }
    
    std::sort(files.begin(), files.end());

    // std::cout << "Files in directory: " << path_str << std::endl;
    // for(const auto& [name, size] : files){
    //     std::cout << "File: " << name << ", Size: " << size << " bytes" << std::endl;
    // }
    // std::cout << "Total files found: " << files.size() << std::endl;
    return files;
}

void mergeCsv(std::string directory, Int_t filesPerStep = 2){
    std::vector<std::pair<std::string, int>> files = dir_lister(directory);

    if (files.size() < filesPerStep) {
        std::cerr << "Not enough files to merge." << std::endl;
        return;
    }

    for (size_t i = 0; i < files.size(); i += filesPerStep) {
        std::cout << "Appending: " << files[i].first << " with ";
        std::fstream firstFile(directory + "/" + files[i].first, std::ios::in | std::ios::out | std::ios::app);
        if (!firstFile) {
            std::cerr << "\nFailed to open file: " << files[i].first << std::endl;
            continue;
        }

        for (size_t j = 1; j < filesPerStep && i + j < files.size(); ++j) {
            std::fstream secondFile(directory + "/" + files[i + j].first, std::ios::in);
            if (!secondFile) {
                std::cerr << "\nFailed to open file: " << files[i + j].first << std::endl;
                continue;
            }
            std::cout << files[i + j].first << "\n";
            // Skip header of the second file
            std::string line;
            std::getline(secondFile, line);
            std::getline(secondFile, line);
            std::getline(secondFile, line);

            while (std::getline(secondFile, line)) {
                firstFile << line << "\n";
            }
            secondFile.close();
        }
        firstFile.close();
    }
    std::cout << "Merging completed." << std::endl;
}

/*The first two arguments are for specifying the filename, its format should be "[directory/fileID]scope_[no].csv".*/

Bool_t saveSignal(std::string fileID = "20250217", std::string no = "21"){
    Bool_t debug = kTRUE;
    gSystem->Load("mySignal_cxx.so");
    std::cout.precision(12);
    
    std::string filename = Form("%sscope_%s.csv", fileID.c_str(), no.c_str());
    std::fstream data;
    data.open(filename, ios::in);

    if(!data){
        std::cout<<"File couldn't be opened!"<<std::endl;
        return kFALSE;
    }

    TFile *signal = new TFile(Form("%s%ssignalSaved.root", fileID.c_str(), no.c_str()), "RECREATE");
    TTree *signTree = new TTree("signTree", "signTree");

    mySignal *ms = new mySignal();
    std::string branchName = Form("signal_%s", no.c_str());
    signTree->Branch(branchName.c_str(), &ms);

    Double_t aMax, aThr, Q, t0, t1, t2, TOT; 

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

        for (Int_t i = 1; i <= signHisto->GetNbinsX(); i++){
            signHisto->AddBinContent(i, -mean);
        }
        
        //-------variance & standrad deviation
        Double_t variance = 0;
        for (Double_t v : baseline) {
            variance += (v - mean) * (v - mean);
        }
        variance /= baseline.size();
        Double_t stddev = std::sqrt(variance);

        //-------amplitude threshold - constant for the entire run.
        aThr = 0.002;
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

        if(j == 1){
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
        }

        ms->set(t0, TOT, aMax, Q);
        if(debug){signHisto->Write();}

        if(TMath::Abs(Q)<0.002){
            signHisto->Write();
            std::cout<<"Q < 0.002!!: "<<Q<<", iter no. "<<j<<std::endl;
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
    signal->Close();

    return kTRUE;
}

Bool_t histosMaking(std::string rootFile = "20250217", std::string no = "21"){
    gStyle->SetOptStat(0);
    
    //opening a file containing the tree and reading it
    std::string filename = Form("%s%ssignalSaved.root", rootFile.c_str(), no.c_str());
    TFile *infile = new TFile(filename.c_str(), "READ");
    if(!infile){
        std::cout<<"Something's wrong! File couldn't be opened!"<<std::endl;
        return kFALSE;
    }

    std::string branchName = Form("signal_%s", no.c_str());
    TTree *tree = (TTree*)infile->Get("signTree");
    if (!tree) {
        std::cout << "Tree not found!" << std::endl;
        return kFALSE;
    }

    Double_t norm = 1;
    if(no == "24") norm = 801613786.2*1e-12;
    if(no == "25") norm = 1120847016.3*1e-12;


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
    Qlow = 0.9*tree->GetMinimum("Q"); aMaxLow = 0.9*tree->GetMinimum("aMax"); TOTlow = 0.9*tree->GetMinimum("TOT"); t0low = 0.9*tree->GetMinimum("t0");
    tree->GetEntry((tree->GetEntries())-1);
    Qup = 1.1*tree->GetMaximum("Q"); aMaxUp = 1.1*tree->GetMaximum("aMax"); TOTup = 1.1*tree->GetMaximum("TOT"); t0up = 1.1*tree->GetMaximum("t0");
    
    //creating histos
    TH1D *Qh = new TH1D("Qh", "Q", 100, -1, 2.5); //
    TH1D *aMaxh = new TH1D("aMaxh", "aMax", 20, aMaxLow, aMaxUp);
    TH1D *TOTh = new TH1D("TOTh", "TOT", 20, TOTlow, TOTup);
    TH1D *t0h = new TH1D("t0h", "t0", 100, t0low, t0up);
    TH2D *aMaxQh = new TH2D("aMaxQh", "aMax vs. Q", 50, aMaxLow, aMaxUp, 50, Qlow, Qup);
    TH2D *Qt0h = new TH2D("Qt0h", "Q vs. t0", 50, Qlow, Qup, 100, t0low, t0up);

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

    TH1D *Qh_copy = (TH1D*)Qh->Clone("Qh_copy");

    if(no == "24"){
        
        Qh_copy->SetTitle("Q; QDC (a.u.); counts/second");
        auto maxVal = Qh_copy->GetMaximum();
        std::cout<<"hej robimy to"<<std::endl;
        for (int i = 1; i <= Qh_copy->GetNbinsX(); ++i) {
            double content = Qh_copy->GetBinContent(i);
            Qh_copy->SetBinContent(i, content * (23196.7/maxVal));
        }

        
    }

    //fitting
    TF1 *aGaus = new TF1("aGaus", "gaus", aMaxLow, aMaxUp);
    TF1 *QGaus = new TF1("QGaus", "gaus", Qlow, Qup);

    aGaus->SetParNames("Constant", "Mean (V)", "Sigma (V)");
    QGaus->SetParNames("Constant", "Mean (C)", "Sigma (C)");

    TFitResultPtr aRes = aMaxh->Fit(aGaus, "S");
    //TFitResultPtr QRes = Qh->Fit(QGaus, "S V", "", Qlow, Qup);


    //drawing and saving the histos
    std::string outFilename = Form("%s_scope_%s_HISTOS.root", rootFile.c_str(), no.c_str());
    TFile *outfile = new TFile(outFilename.c_str(), "RECREATE");

    std::string aParStr = Form("#splitline{mean = (%.5f #pm %.5f) V}{#splitline{sigma = (%.5f #pm %.5f) V}{constant = (%.0f #pm %.0f)}}",
                               aGaus->GetParameter(1), aGaus->GetParError(1),
                               aGaus->GetParameter(2), aGaus->GetParError(2),
                               aGaus->GetParameter(0), aGaus->GetParError(0));
    TLatex *aMaxPars = new TLatex();

    std::string QParStr = Form("#splitline{mean = (%.3f #pm %.3f) C}{#splitline{sigma = (%.4f #pm %.4f) C}{constant = (%.0f #pm %.0f)}}",
                               QGaus->GetParameter(1), QGaus->GetParError(1),
                               QGaus->GetParameter(2), QGaus->GetParError(2),
                               QGaus->GetParameter(0), QGaus->GetParError(0));
    TLatex *QPars = new TLatex();
    //std::cout<<"QPars "<<QParStr<<std::endl;

    Qh_copy->Write();

    TCanvas *c1 = new TCanvas();
    c1->Divide(2, 2);

    c1->cd(1);
    gStyle->SetOptStat(1);
    t0h->SetTitle("t0; t0 (ps); counts");
    t0h->Draw();
    t0h->Write();

    c1->cd(2);
    gStyle->SetOptStat(1);
    TOTh->SetTitle("TOT; TOT (ps); counts");
    TOTh->Sumw2();
    TOTh->Draw();
    TOTh->Write();

    c1->cd(3);
    gStyle->SetOptStat(1);
    gStyle->SetOptFit(111);
    Qh->SetTitle("Q; Q (C); counts/second");
    
    for (int i = 1; i <= Qh->GetNbinsX(); ++i) {
        double content = Qh->GetBinContent(i);
        std::cout<<content<<std::endl;
        Qh->SetBinContent(i, content / norm);
    }
    //Qh->Sumw2();
    Qh->Draw();
    //QPars->DrawLatexNDC(0.55, 0.8, QParStr.c_str()); // Use DrawLatexNDC for TLatex
    Qh->Write();

    c1->cd(4);
    gStyle->SetOptStat(1);
    gStyle->SetOptFit(111);
    aMaxh->SetTitle("aMax; aMax (V); counts");
    //aMaxh->Sumw2();
    aMaxh->Draw();
    //aMaxPars->DrawLatexNDC(0.55, 0.8, aParStr.c_str()); // Use DrawLatexNDC for TLatex
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

void saveMulti(){
    for(int i = 1; i<16; i=i+1){
        saveSignal("/scratch3/lhcb/data/firstTestWithScopeApr25/250410_", std::to_string(i));
    }
}

void drawMulti(){
    for(int i = 1; i<16; i=i+1){
        histosMaking("/scratch3/lhcb/data/firstTestWithScopeApr25/250410_", std::to_string(i));
    }
}
