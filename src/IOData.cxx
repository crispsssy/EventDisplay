#include "IOData.hxx"

IOData::IOData(){}

IOData::~IOData(){
	if(f) f->Close();
	if(fHits){
		for(auto hit = fHits->begin(); hit != fHits->end(); ++hit){
			delete (*hit);
		}
	}
}

IOData* IOData::fIOData = nullptr;

IOData& IOData::Get(){
	if(!fIOData){
		fIOData = new IOData();
	}
	return *fIOData;
}

bool IOData::ReadFile(std::string filePath){
	f = new TFile(filePath.c_str(), "READ");
	t = (TTree*)f->Get("RECBE");
	if(!t){
		std::cerr<<"Failed to get TTree from file! Maybe it's not a root file"<<std::endl;
		return false;
	}
	
	t->SetBranchAddress("adc", &adc);
	t->SetBranchAddress("tdcNhit", &tdcNhit);
	t->SetBranchAddress("tdcDiff", &tdcDiff);
	return true;
}

bool IOData::Preprocess(int numEvent){
	if(!t){
		std::cerr<<"TTree not set yet!!!"<<std::endl;
		return false;
	}
	TH1S h_pedestal[4992];
	for(int i=0; i<4992; ++i){
		h_pedestal[i] = TH1S(Form("h_pedestal_%d", i), Form("h_pedestal %d", i), 100, 150, 250);
	}
	h_tdc = new TH1I("h_tdc", "h_tdc", 4000, -3000, 1000);
	if(numEvent > t->GetEntries()) numEvent = t->GetEntries();
	for(int iEvent = 0; iEvent < numEvent; ++iEvent){
		t->GetEntry(iEvent);
		for(int iCh = 0; iCh < 4992; ++iCh){
			if(tdcNhit[iCh] == 0){
				for(int iSample = 0; iSample < 5; ++iSample){
					if(adc[iCh][iSample] == 0) break;
					h_pedestal[iCh].Fill(adc[iCh][iSample]);
				}
			}
			else{
				h_tdc->Fill(tdcDiff[iCh][0]);
			}
		}
	}

	//pedestal
	for(int iCh = 0; iCh < 4992; ++iCh){
		fPedestal[iCh] = h_pedestal[iCh].GetXaxis()->GetBinCenter(h_pedestal[iCh].GetMaximumBin());
		if(fPedestal[iCh] < 150) fPedestal[iCh] = 999;
	}
	//TDC min value
	for(int ibin = 1; ibin <= h_tdc->GetNbinsX(); ++ibin){
		if(h_tdc->GetBinContent(ibin) > 0){
			fTDCMinValue = h_tdc->GetBinLowEdge(ibin);
			break;
		}
	}
	//T0
	double peakX = h_tdc->GetXaxis()->GetBinCenter(h_tdc->GetMaximumBin());
	TF1* sigmoid = new TF1("sigmoid", "[0]/(1+exp(([1]-x)/[2]))");
	sigmoid->SetParameter(0, h_tdc->GetMaximum());
	sigmoid->SetParameter(1, peakX);
	sigmoid->SetParameter(2, 1.);
	h_tdc->Fit(sigmoid, "Q", "", peakX - 100, peakX + 300);

	TF1* exponential = new TF1("exponential", "[0]*exp([1]*(x-[2]))");
	exponential->SetParameter(0, sigmoid->GetParameter(0));
	exponential->SetParameter(2, sigmoid->GetParameter(1));
	h_tdc->Fit(exponential, "Q", "", peakX - 100, peakX + 300);

	f_T0 = new TF1("f_T0", "[0]+[1]*(exp([2]*(x-[3]))/(1+exp(([4]-x)/[5])))");
	f_T0->SetParameter(0,0.);
        f_T0->SetParameter(1,sigmoid->GetParameter(0));
        f_T0->SetParameter(2,exponential->GetParameter(1));
        f_T0->SetParameter(3,sigmoid->GetParameter(1));
        f_T0->SetParameter(4,sigmoid->GetParameter(1));
        f_T0->SetParameter(5,sigmoid->GetParameter(2));
        h_tdc->Fit(f_T0, "Q", "", peakX - 100, peakX + 300);
        fT0 = f_T0->GetParameter(4) - fTDCMinValue;

	return true;
}
	

CDCHitContainer* IOData::GetHits(int event){
	if(fHits){
		for(auto hit = fHits->begin(); hit != fHits->end(); ++hit){
			delete (*hit);
		}
		delete fHits;
		fHits = nullptr;
	}
	if(!t){
		std::cerr<<"Can not find TTree, did you forget to read root file?"<<std::endl;
		exit(-1);
	}
	
	fHits = new CDCHitContainer();
	t->GetEntry(event);
	for(int iCh=0; iCh<4992; ++iCh){
		if(tdcNhit[iCh] > 0){
			CDCHit* hit = new CDCHit(iCh);
			int sum = 0;
			int sum_woUnder = 0;
			for(int iSample = 0; iSample < 32; ++iSample){
				hit->InsertADC(adc[iCh][iSample]);
				if(tdcDiff[iCh][iSample] != 0) hit->InsertTDC(tdcDiff[iCh][iSample] - fTDCMinValue);
				if(adc[iCh][iSample] > fPedestal[iCh]) sum_woUnder += adc[iCh][iSample] - fPedestal[iCh];
				sum += adc[iCh][iSample] - fPedestal[iCh];
			}
			hit->SetADCSumWithUnderShoot(sum);
			hit->SetADCSumWithoutUnderShoot(sum_woUnder);
			fHits->push_back(hit);
		}
	}
	return fHits;
}
