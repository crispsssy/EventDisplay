#ifndef _IODATA_HXX_
#define _IODATA_HXX_

#include <iostream>
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include "CDCHit.hxx"

class IOData{
public:
	~IOData();
	static IOData& Get();
	
	bool ReadFile(std::string filePath);
	bool Preprocess(int numEvent);
	CDCHitContainer* GetHits(int event);
	int GetTDCMinValue() const { return fTDCMinValue; }
	int GetNumberOfEvent() const { return t->GetEntries(); }

private:
	IOData();
	IOData(IOData const& src);
	IOData& operator=(IOData const& rhs);

	static IOData* fIOData;

	TFile* f = nullptr;
	TTree* t = nullptr;
	CDCHitContainer* fHits = nullptr;
	short fPedestal[4992] = {0};
	int fTDCMinValue = 0;
	short adc[4992][32];
	short tdcNhit[4992];
	int tdcDiff[4992][32];
};

#endif
