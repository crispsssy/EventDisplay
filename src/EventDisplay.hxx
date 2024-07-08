#ifndef EVENTDISPLAY_HXX
#define EVENTDISPLAY_HXX

#include <TApplication.h>
#include <TTimer.h>
#include <TGClient.h>
#include <TGFrame.h>
#include <TGButton.h>
#include <TGFileDialog.h>
#include <TGNumberEntry.h>
#include <TGLabel.h>
#include <TGTextView.h>
#include <TGComboBox.h>
#include <TRootEmbeddedCanvas.h>
#include <TROOT.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <TVector2.h>
#include <TVector3.h>
#include <TEllipse.h>
#include <TMarker.h>
#include <RQ_OBJECT.h>
#include <iostream>
#include <vector>

#include "IOData.hxx"
#include "CDCHit.hxx"
#include "CDCGeom.hxx"

class EventDisplay {
	RQ_OBJECT("EventDisplay")

public:
	EventDisplay();
	virtual ~EventDisplay();
	void SetupMainWindow();
	void CloseMainWindow();
	void OpenFileDialog();
	void Preprocess();
	void ErrorMessage(std::string message);
	void SetupDisplay();
	void IsOddEvenSelected(int isOddEvenEntry);
	void EntryNumberChanged();
	void ZPosSelected(int zPosEntry);
	void UpdateWaveforms();
	void UpdateGraph();
	void DrawCDCXY();
	void ShowWaveform(TVirtualPad* pad, TObject* obj, Int_t ihp, Int_t y);

private:
	TGMainFrame* mainFrame = nullptr;
	TGNumberEntry* preNumEventEntry = nullptr;
	TGTextButton* processButton = nullptr;
	TGLabel* error_label = nullptr;
	TRootEmbeddedCanvas* embeddedCanvas = nullptr;
	TCanvas* canvas = nullptr;
	TGComboBox* boxIsOddEven = nullptr;
	TGComboBox* boxZPos = nullptr;
	TGNumberEntry* eventEntry = nullptr;

	std::string fFileName;
	int fRunNum = 0;
	int fEvent = 0;
	int fIsOddEvenEntry = 0;
	int fZPosEntry = 0;
	CDCHitContainer* fHits;
	TCanvas* c_waveform = nullptr;
	TGraph* graph = nullptr;
	TGraph* graph_odd = nullptr;
	TGraph* graph_even = nullptr;
	static TEllipse* cdcOuterWall;
	static TEllipse* cdcInnerWall;
	std::map<int, TGraph*> g_waveforms;
	std::map<int, std::vector<TMarker>> fTDCMarkers;
};

#endif // EVENTDISPLAY_HXX

