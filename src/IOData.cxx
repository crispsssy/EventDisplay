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
	TH1I h_tdc("h_tdc", "h_tdc", 4000, -3000, 1000);
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
				h_tdc.Fill(tdcDiff[iCh][0]);
			}
		}
	}

	//pedestal
	for(int iCh = 0; iCh < 4992; ++iCh){
		fPedestal[iCh] = h_pedestal[iCh].GetXaxis()->GetBinCenter(h_pedestal[iCh].GetMaximumBin());
		if(fPedestal[iCh] < 150) fPedestal[iCh] = 999;
	}
	//TDC min value
	for(int ibin = 1; ibin <= h_tdc.GetNbinsX(); ++ibin){
		if(h_tdc.GetBinContent(ibin) > 0){
			fTDCMinValue = h_tdc.GetBinLowEdge(ibin);
			break;
		}
	}
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
			for(int iSample = 0; iSample < 32; ++iSample){
				hit->InsertADC(adc[iCh][iSample]);
				if(tdcDiff[iCh][iSample] != 0) hit->InsertTDC(tdcDiff[iCh][iSample] - fTDCMinValue);
			}
			fHits->push_back(hit);
		}
	}
	return fHits;
}
