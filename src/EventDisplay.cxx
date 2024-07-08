#include "EventDisplay.hxx"

TEllipse* EventDisplay::cdcOuterWall = nullptr;
TEllipse* EventDisplay::cdcInnerWall = nullptr;

EventDisplay::EventDisplay() {
	SetupMainWindow();
}

void EventDisplay::SetupMainWindow(){
	// Create main frame
	mainFrame = new TGMainFrame(gClient->GetRoot(), 800, 600);
	
	//Add open file button
	TGTextButton* openFileButton = new TGTextButton(mainFrame, "Select File");
	mainFrame->AddFrame(openFileButton, new TGLayoutHints(kLHintsCenterY, 5, 5, 10, 10));

	// Add label
	TGLabel* label = new TGLabel(mainFrame, "Please select number of entries that you want to preprocess for T0 and ADC pedestal value");
	mainFrame->AddFrame(label, new TGLayoutHints(kLHintsCenterX, 5, 5, 10, 10));
	
	// Add number entry
	preNumEventEntry = new TGNumberEntry(mainFrame, 100, 9, -1, TGNumberFormat::kNESInteger);
	mainFrame->AddFrame(preNumEventEntry, new TGLayoutHints(kLHintsCenterX, 5, 5, 10, 10));

	// Add process button
	processButton = new TGTextButton(mainFrame, "&Process");
	mainFrame->AddFrame(processButton, new TGLayoutHints(kLHintsCenterX, 5, 5, 10, 10));

	//Add error message frame
	error_label = new TGLabel(mainFrame, "Press to process");
	mainFrame->AddFrame(error_label, new TGLayoutHints(kLHintsCenterX, 5, 5, 10, 10));

	//Connections
	openFileButton->Connect("Clicked()", "EventDisplay", this, "OpenFileDialog()");
	preNumEventEntry->GetNumberEntry()->Connect("ReturnPressed()", "TGTextButton", processButton, "Clicked()");
	processButton->Connect("Clicked()", "EventDisplay", this, "Preprocess()");
	mainFrame->Connect("CloseWindow()", "EventDisplay", this, "CloseMainWindow()");

	mainFrame->SetWindowName("Preprocess Entries");
	mainFrame->MapSubwindows();
	mainFrame->Resize(mainFrame->GetDefaultSize());
	mainFrame->MapWindow();
}

EventDisplay::~EventDisplay() {
	mainFrame->Cleanup();
	delete mainFrame;
}

void EventDisplay::CloseMainWindow(){
	gApplication->Terminate(0);
}

void EventDisplay::OpenFileDialog(){
	TGFileInfo fileInfo;
	char const* fileTypes[] = {"All files", "*", 0, 0};
	static TString iniDir("..");
	fileInfo.fFileTypes = fileTypes;
	fileInfo.fIniDir = StrDup(iniDir);
	TGFileDialog* fileDialog = new TGFileDialog(gClient->GetRoot(), mainFrame, kFDOpen, &fileInfo);
	if(fileInfo.fFilename){
		std::cout<<"Selected file: "<<fileInfo.fFilename<<std::endl;
		fFileName = fileInfo.fFilename;
	}
}

void EventDisplay::Preprocess() {
	int numEvent = preNumEventEntry->GetIntNumber();
	std::cout<<"file name is"<<std::endl;
	std::cout << "Processing " << numEvent << " entries..." << std::endl;
	if(fFileName.size() == 0){
		ErrorMessage("Please choose a file!!!");
		return;
	}
	else{
		std::string::size_type idx = fFileName.find("run0");
		if(idx == std::string::npos){
			ErrorMessage("Not a data file with run number!!!");
			return;
		}
		else{
			fRunNum = stoi(fFileName.substr(idx+4, 4));
		}
	}
	if(numEvent < 100){
		ErrorMessage("Please at least choose 100 events to get the T0 and pedestal value!!!");
		return;
	}
	ErrorMessage("Processing...");
	IOData::Get().ReadFile(fFileName);
	IOData::Get().Preprocess(numEvent);

	// Call SetupDisplay to create new elements
	TTimer::SingleShot(100, "EventDisplay", this, "SetupDisplay()");
}

void EventDisplay::ErrorMessage(std::string message){
	std::cout<<"Error message: "<<message<<std::endl;
	error_label->SetText(message.c_str());
	error_label->SetTextColor(0xFF0000);
	mainFrame->Layout();
}

void EventDisplay::SetupDisplay() {
	mainFrame->Cleanup();
	// Create embedded canvas
	embeddedCanvas = new TRootEmbeddedCanvas("embeddedCanvas", mainFrame, 800, 600);
	mainFrame->AddFrame(embeddedCanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
	canvas = embeddedCanvas->GetCanvas();

	// Set new canvas size
	canvas->SetCanvasSize(800, 600);
	canvas->cd();

	// Create and draw a TGraph
	std::cout<<"Try to get hits from initial event number "<<fEvent<<std::endl;
	fHits = IOData::Get().GetHits(fEvent);
	if(!fHits) std::cerr<<"Failed to get hits!"<<std::endl;
	UpdateWaveforms();
	UpdateGraph();

	TGHorizontalFrame* displayOptionFrame = new TGHorizontalFrame(mainFrame, 800, 30);
	TGHorizontalFrame* tdcOptionFrame = new TGHorizontalFrame(mainFrame, 800, 30);
	TGHorizontalFrame* adcSumWithUndershootOptionFrame = new TGHorizontalFrame(mainFrame, 800, 30);
	TGHorizontalFrame* adcSumWithoutUndershootOptionFrame = new TGHorizontalFrame(mainFrame, 800, 30);
	// Add first TGComboBox
	boxIsOddEven = new TGComboBox(displayOptionFrame, "boxIsOddEven");
	boxIsOddEven->AddEntry("all hits", 0);
	boxIsOddEven->AddEntry("odd/even layers", 1);
	displayOptionFrame->AddFrame(boxIsOddEven, new TGLayoutHints(kLHintsBottom | kLHintsLeft, 5, 5, 10, 10));
	boxIsOddEven->Select(0);
	boxIsOddEven->Resize(150, 20);

	// Add additional TGNumberEntry
	TGLabel* entryLabel = new TGLabel(displayOptionFrame, "Entry");
	displayOptionFrame->AddFrame(entryLabel, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 0, 0, 10, 10));
	eventEntry = new TGNumberEntry(displayOptionFrame, 0, 9, -1, TGNumberFormat::kNESInteger);
	displayOptionFrame->AddFrame(eventEntry, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));

	// Add second TGComboBox
	boxZPos = new TGComboBox(displayOptionFrame, "boxZPos");
	boxZPos->AddEntry("RO", 0);
	boxZPos->AddEntry("Z0", 1);
	boxZPos->AddEntry("HV", 2);
	displayOptionFrame->AddFrame(boxZPos, new TGLayoutHints(kLHintsBottom | kLHintsRight, 5, 5, 10, 10));
	boxZPos->Select(0);
	boxZPos->Resize(150, 20);

	// Add TDC filter TGNumberEntry
	T0Label = new TGLabel(tdcOptionFrame, Form("(T0 = %d)", IOData::Get().GetT0()));
	T0Label->SetTextColor(0xFF0000);
	TGLabel* tdcLabel = new TGLabel(tdcOptionFrame, "TDC range");
	tdcMinEntry = new TGNumberEntry(tdcOptionFrame, fTDCMin, 9, -1, TGNumberFormat::kNESInteger);
	tdcMaxEntry = new TGNumberEntry(tdcOptionFrame, fTDCMax, 9, -1, TGNumberFormat::kNESInteger);
	tdcOptionFrame->AddFrame(T0Label, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 40, 40, 10, 10));
	tdcOptionFrame->AddFrame(tdcLabel, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));
	tdcOptionFrame->AddFrame(tdcMinEntry, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));
	tdcOptionFrame->AddFrame(tdcMaxEntry, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));

	// Add ADC sum with undershoot filter
	TGLabel* adcSumWithUndershootLabel = new TGLabel(adcSumWithUndershootOptionFrame, "ADC sum with undershoot range");
	adcSumWithUndershootMinEntry = new TGNumberEntry(adcSumWithUndershootOptionFrame, fADCSumWUnderMin, 9, -1, TGNumberFormat::kNESInteger);
	adcSumWithUndershootMaxEntry = new TGNumberEntry(adcSumWithUndershootOptionFrame, fADCSumWUnderMax, 9, -1, TGNumberFormat::kNESInteger);
	adcSumWithUndershootOptionFrame->AddFrame(adcSumWithUndershootLabel, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 15, 15, 10, 10));
	adcSumWithUndershootOptionFrame->AddFrame(adcSumWithUndershootMinEntry, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));
	adcSumWithUndershootOptionFrame->AddFrame(adcSumWithUndershootMaxEntry, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));

	// Add ADC sum without undershoot filter
	TGLabel* adcSumWithoutUndershootLabel = new TGLabel(adcSumWithoutUndershootOptionFrame, "ADC sum without undershoot range");
	adcSumWithoutUndershootMinEntry = new TGNumberEntry(adcSumWithoutUndershootOptionFrame, fADCSumWOUnderMin, 9, -1, TGNumberFormat::kNESInteger);
	adcSumWithoutUndershootMaxEntry = new TGNumberEntry(adcSumWithoutUndershootOptionFrame, fADCSumWOUnderMax, 9, -1, TGNumberFormat::kNESInteger);
	adcSumWithoutUndershootOptionFrame->AddFrame(adcSumWithoutUndershootLabel, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));
	adcSumWithoutUndershootOptionFrame->AddFrame(adcSumWithoutUndershootMinEntry, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));
	adcSumWithoutUndershootOptionFrame->AddFrame(adcSumWithoutUndershootMaxEntry, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));

	mainFrame->AddFrame(adcSumWithoutUndershootOptionFrame, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));
	mainFrame->AddFrame(adcSumWithUndershootOptionFrame, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));
	mainFrame->AddFrame(tdcOptionFrame, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));
	mainFrame->AddFrame(displayOptionFrame, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));

	//Connections
	boxIsOddEven->Connect("Selected(Int_t)",                       "EventDisplay", this, "IsOddEvenSelected(int)");
	eventEntry->Connect("ValueSet(Long_t)",                        "EventDisplay", this, "EntryNumberChanged()");
	boxZPos->Connect("Selected(Int_t)",                            "EventDisplay", this, "ZPosSelected(int)");
	tdcMinEntry->Connect("ValueSet(Long_t)",                       "EventDisplay", this, "FilterChanged()");
	tdcMaxEntry->Connect("ValueSet(Long_t)",                       "EventDisplay", this, "FilterChanged()");
	adcSumWithUndershootMinEntry->Connect("ValueSet(Long_t)",      "EventDisplay", this, "FilterChanged()");
	adcSumWithUndershootMaxEntry->Connect("ValueSet(Long_t)",      "EventDisplay", this, "FilterChanged()");
	adcSumWithoutUndershootMinEntry->Connect("ValueSet(Long_t)",   "EventDisplay", this, "FilterChanged()");
	adcSumWithoutUndershootMaxEntry->Connect("ValueSet(Long_t)",   "EventDisplay", this, "FilterChanged()");
	// Set window name and map subwindows, main window
	mainFrame->SetWindowName("Event Display");
	mainFrame->MapSubwindows();
	mainFrame->Resize(mainFrame->GetDefaultSize());
	mainFrame->MapWindow();
}

void EventDisplay::IsOddEvenSelected(int isOddEvenEntry){
	std::cout<<"IsOddEven selected"<<std::endl;
	if(isOddEvenEntry != fIsOddEvenEntry){
		fIsOddEvenEntry = isOddEvenEntry;
		UpdateGraph();
	}
}

void EventDisplay::EntryNumberChanged(){
	std::cout<<"Entry changed"<<std::endl;
	int event = eventEntry->GetIntNumber();
	if(event >= IOData::Get().GetNumberOfEvent()){
		std::cerr<<"Exceed max event number!!!"<<std::endl;
		return;
	}
	if(event < 0){
		std::cerr<<"Invalid event number!!!"<<std::endl;
		return;
	}
	if(fEvent != event){
		fEvent = event;
		std::cout<<"event number is "<<fEvent<<std::endl;
		fHits = IOData::Get().GetHits(fEvent);
		UpdateWaveforms();
		UpdateGraph();
	}
}

void EventDisplay::ZPosSelected(int zPosEntry){
	std::cout<<"Z position selected"<<std::endl;
	if(fZPosEntry != zPosEntry){
		fZPosEntry = zPosEntry;
		UpdateGraph();
	}
}

void EventDisplay::FilterChanged(){
	std::cout<<"Filter option chenge detected"<<std::endl;
	int tdcMin = tdcMinEntry->GetIntNumber();
	int tdcMax = tdcMaxEntry->GetIntNumber();
	int adcSumWUnderMin = adcSumWithUndershootMinEntry->GetIntNumber();
	int adcSumWUnderMax = adcSumWithUndershootMaxEntry->GetIntNumber();
	int adcSumWOUnderMin = adcSumWithoutUndershootMinEntry->GetIntNumber();
	int adcSumWOUnderMax = adcSumWithoutUndershootMaxEntry->GetIntNumber();
	if(fTDCMin != tdcMin || fTDCMax != tdcMax || 
		fADCSumWUnderMin != adcSumWUnderMin || fADCSumWUnderMax != adcSumWUnderMax ||
		fADCSumWOUnderMin != adcSumWOUnderMin || fADCSumWOUnderMax != adcSumWOUnderMax){
		fTDCMin = tdcMin;
		fTDCMax = tdcMax;
		fADCSumWUnderMin = adcSumWUnderMin;
		fADCSumWUnderMax = adcSumWUnderMax;
		fADCSumWOUnderMin = adcSumWOUnderMin;
		fADCSumWOUnderMax = adcSumWOUnderMax;
		UpdateGraph();
	}
}

void EventDisplay::UpdateWaveforms(){
	std::cout<<"Update waveform graphs"<<std::endl;
	//release memory of previous waveforms
	for(auto waveform = g_waveforms.begin(); waveform != g_waveforms.end(); ++waveform){
		delete waveform->second;
		waveform->second = nullptr;
	}
	fTDCMarkers.clear();
	fTDCMarkerText.clear();
	//fill new waveforms
	for(auto hit = fHits->begin(); hit != fHits->end(); ++hit){
		int channel = (*hit)->GetChannelID();
		int layer = CDCGeom::Get().ChannelToLayer(channel);
		TGraph* g = new TGraph();
		g->SetName(Form("g_%d", channel));
		g->SetTitle(Form("waveform of ch %d   layer == %d", channel, layer));
		g->SetMarkerStyle(58);
		g->SetMarkerColor(kRed);
		g->GetXaxis()->SetTitle("ADC sample index");
		g->GetYaxis()->SetTitle("ADC value");
		std::vector<short> adcs = (*hit)->GetADC();
		for(auto adc = adcs.begin(); adc != adcs.end(); ++adc){
			g->AddPoint(adc - adcs.begin(), *adc);
		}
		std::vector<int> tdcs = (*hit)->GetTDC();
		for(auto tdc = tdcs.begin(); tdc != tdcs.end(); ++tdc){
			double x = (double)(*tdc) / 32;
			double y = 0;
			TMarker m(x, y, 22);
			m.SetMarkerColor(kRed);
			m.SetMarkerSize(1);
			fTDCMarkers[channel].push_back(m);
			TText tt(x, y, Form("%d", *tdc));
			tt.SetTextSize(0.035);
			tt.SetTextAlign(22);
			tt.SetTextColor(kRed);
			fTDCMarkerText[channel].push_back(tt);
		}
		TText tt_withUnder(0.7, 0.85, Form("ADC sum with undershoot = %d", (*hit)->GetADCSumWithUnderShoot()));
		tt_withUnder.SetTextSize(0.035);
		tt_withUnder.SetTextAlign(22);
		tt_withUnder.SetNDC();
		TText tt_withoutUnder(0.7, 0.8, Form("ADC sum w/o undershoot = %d", (*hit)->GetADCSumWithoutUnderShoot()));
		tt_withoutUnder.SetTextSize(0.035);
		tt_withoutUnder.SetTextAlign(22);
		tt_withoutUnder.SetNDC();
		fADCSumText[channel] = std::pair<TText, TText>(tt_withUnder, tt_withoutUnder);
		g_waveforms[channel] = g;
	}
}

void EventDisplay::UpdateGraph(){
	auto store = gPad;
	std::cout<<"Update the event display"<<std::endl;
	canvas->cd();
	canvas->Clear();
	canvas->Update();

	canvas->Connect("Highlighted(TVirtualPad*, TObject*, Int_t, Int_t)", "EventDisplay", this, "ShowWaveform(TVirtualPad*, TObject*, Int_t, Int_t)");

	if(fIsOddEvenEntry == 0){
		if(graph){
			delete graph;
			graph = nullptr;
		}
		if(graph_filtered){
			delete graph_filtered;
			graph_filtered = nullptr;
		}
		canvas->SetCanvasSize(800, 600);
		TH1F* frame = gPad->DrawFrame(-850., -850., 850., 850.);
		frame->SetTitle(Form("EventDisplay run%d entry %d", fRunNum, fEvent));
		frame->GetXaxis()->SetTitle("X [mm]");
		frame->GetYaxis()->SetTitle("Y [mm]");
		DrawCDCXY();

		graph = new TGraph();
		graph->SetMarkerStyle(20);
		graph->SetMarkerSize(0.4);
		graph->SetMarkerColor(kBlue);
		graph_filtered = new TGraph();
		graph_filtered->SetMarkerStyle(20);
		graph_filtered->SetMarkerSize(0.4);
		graph_filtered->SetMarkerColor(kBlue-10);
		for(std::vector<CDCHit*>::const_iterator hit = fHits->begin(); hit != fHits->end(); ++hit){
			int channel = (*hit)->GetChannelID();
			TVector2 pos;
			if(fZPosEntry == 0) pos = CDCGeom::Get().ChannelToROPos(channel);
			else if(fZPosEntry == 1) pos = CDCGeom::Get().ChannelToZ0Pos(channel);
			else if(fZPosEntry == 2) pos = CDCGeom::Get().ChannelToHVPos(channel);
			else{ std::cerr<<"Error of z position entry!!!"<<std::endl; exit(-1); }
			if(IsFiltered(*hit)) graph_filtered->SetPoint(channel, pos.X(), pos.Y());
			else graph->SetPoint(channel, pos.X(), pos.Y());
		}
		for(int i=0; i<4992; ++i){
			double x = 0, y = 0;
			graph->GetPoint(i, x, y);
			if(x == 0 && y == 0) graph->SetPoint(i, 99999, 99999);
			x = 0; y = 0;
			graph_filtered->GetPoint(i, x, y);
			if(x == 0 && y == 0) graph_filtered->SetPoint(i, 99999, 99999);
		}
		graph_filtered->Draw("P");
		graph->Draw("P");
		gPad->Update();
		graph->SetHighlight();
		graph_filtered->SetHighlight();
	}
	else{
		if(graph_odd){
			delete graph_odd;
			graph_odd = nullptr;
		}
		if(graph_even){
			delete graph_even;
			graph_even = nullptr;
		}
		if(graph_odd_filtered){
			delete graph_odd_filtered;
			graph_odd_filtered = nullptr;
		}
		if(graph_even_filtered){
			delete graph_even_filtered;
			graph_even_filtered = nullptr;
		}
		canvas->SetCanvasSize(800, 400);
		canvas->Divide(2,1);
		graph_odd = new TGraph();
		graph_odd->SetMarkerStyle(20);
		graph_odd->SetMarkerSize(0.4);
		graph_odd->SetMarkerColor(kBlue);
		graph_odd_filtered = new TGraph();
		graph_odd_filtered->SetMarkerStyle(20);
		graph_odd_filtered->SetMarkerSize(0.4);
		graph_odd_filtered->SetMarkerColor(kBlue-10);
		graph_even = new TGraph();
		graph_even->SetMarkerStyle(20);
		graph_even->SetMarkerSize(0.4);
		graph_even->SetMarkerColor(kBlue);
		graph_even_filtered = new TGraph();
		graph_even_filtered->SetMarkerStyle(20);
		graph_even_filtered->SetMarkerSize(0.4);
		graph_even_filtered->SetMarkerColor(kBlue-10);
		for(std::vector<CDCHit*>::const_iterator hit = fHits->begin(); hit != fHits->end(); ++hit){
			int channel = (*hit)->GetChannelID();
			int layer = CDCGeom::Get().ChannelToLayer(channel);
			TVector2 pos;
			if(fZPosEntry == 0) pos = CDCGeom::Get().ChannelToROPos(channel);
			else if(fZPosEntry == 1) pos = CDCGeom::Get().ChannelToZ0Pos(channel);
			else if(fZPosEntry == 2) pos = CDCGeom::Get().ChannelToHVPos(channel);
			else{ std::cerr<<"Error of z position entry!!!"<<std::endl; exit(-1); }

			if(layer % 2 == 1){
				if(IsFiltered(*hit)) graph_odd_filtered->SetPoint(channel, pos.X(), pos.Y());
				else graph_odd->SetPoint(channel, pos.X(), pos.Y());
			}
			else{
				if(IsFiltered(*hit)) graph_even_filtered->SetPoint(channel, pos.X(), pos.Y());
				else graph_even->SetPoint(channel, pos.X(), pos.Y());
			}
		}
		for(int i=0; i<4992; ++i){
			double x = 0, y = 0;
			graph_odd->GetPoint(i, x, y);
			if(x == 0 && y == 0) graph_odd->SetPoint(i, 99999, 99999);
			x = 0; y = 0;
			graph_odd_filtered->GetPoint(i, x, y);
			if(x == 0 && y == 0) graph_odd_filtered->SetPoint(i, 99999, 99999);
			x = 0, y = 0;
			graph_even->GetPoint(i, x, y);
			if(x == 0 && y == 0) graph_even->SetPoint(i, 99999, 99999);
			x = 0; y = 0;
			graph_even_filtered->GetPoint(i, x, y);
			if(x == 0 && y == 0) graph_even_filtered->SetPoint(i, 99999, 99999);
		}
		canvas->cd(1);
		TH1F* frame_odd = gPad->DrawFrame(-850., -850., 850., 850.);
		frame_odd->SetTitle(Form("EventDisplay for odd layers at run%d entry %d", fRunNum, fEvent));
		frame_odd->GetXaxis()->SetTitle("X [mm]");
		frame_odd->GetYaxis()->SetTitle("Y [mm]");
		DrawCDCXY();
		graph_odd_filtered->Draw("P");
		graph_odd->Draw("P");
		gPad->SetLeftMargin(0.13);
		gPad->Update();
		graph_odd->SetHighlight();
		graph_odd_filtered->SetHighlight();
		canvas->cd(2);
		TH1F* frame_even = gPad->DrawFrame(-850., -850., 850., 850.);
		frame_even->SetTitle(Form("EventDisplay for even layers at run%d entry %d", fRunNum, fEvent));
		frame_even->GetXaxis()->SetTitle("X [mm]");
		frame_even->GetYaxis()->SetTitle("Y [mm]");
		DrawCDCXY();
		graph_even_filtered->Draw("P");
		graph_even->Draw("P");
		gPad->SetLeftMargin(0.13);
		gPad->Update();
		graph_even->SetHighlight();
		graph_even_filtered->SetHighlight();
	}

	mainFrame->MapSubwindows();
        mainFrame->MapWindow();
	gPad = store;
}

bool EventDisplay::IsFiltered(CDCHit* hit){
	//tdc filter
	int channel = hit->GetChannelID();
	std::vector<int> tdcs = hit->GetTDC();
	if(*tdcs.begin() > fTDCMax || *tdcs.rbegin() < fTDCMin){
		std::cout<<"Channel "<<channel<<" filtered by tdc "<<*tdcs.begin()<<" to "<<*tdcs.rbegin()<<std::endl;
		return true;
	}
	//adc sum with undershoot filter
	int sum = hit->GetADCSumWithUnderShoot();
	if(sum > fADCSumWUnderMax || sum < fADCSumWUnderMin){
		std::cout<<"Channel "<<channel<<" filtered by adc sum with undershoot "<<sum<<std::endl;
		return true;
	}
	//adc sum without undershoot filter
	int sum_woUnder = hit->GetADCSumWithoutUnderShoot();
	if(sum_woUnder > fADCSumWOUnderMax || sum_woUnder < fADCSumWOUnderMin){
		std::cout<<"Channel "<<channel<<" filtered by adc sum without undershoot "<<sum_woUnder<<std::endl;
		return true;
	}

	return false;
}

void EventDisplay::DrawCDCXY(){
	if(!cdcOuterWall) cdcOuterWall = new TEllipse(0, 0, 840.);
        if(!cdcInnerWall) cdcInnerWall = new TEllipse(0, 0, 496.);
        cdcOuterWall->Draw();
        cdcInnerWall->Draw();
}

void EventDisplay::ShowWaveform(TVirtualPad* pad, TObject* obj, Int_t ihp, Int_t y){
	auto store = gPad;
	c_waveform = (TCanvas*)gROOT->GetListOfCanvases()->FindObject("c_waveform");
        if(!c_waveform){
                std::cout<<"Make new Canvas for ADC waveform"<<std::endl;
                c_waveform = new TCanvas("c_waveform", "ADC waveform", 505, 0, 600, 400);
        }

        if(ihp == -1){
                delete c_waveform;
                return;
        }

        if(g_waveforms.size() != 0 && g_waveforms.find(ihp) != g_waveforms.end()){
                c_waveform->cd();
		c_waveform->Clear();
		c_waveform->Update();
                g_waveforms.at(ihp)->Draw("APL");
		double y = g_waveforms.at(ihp)->GetYaxis()->GetXmin();
		double yRange = g_waveforms.at(ihp)->GetYaxis()->GetXmax() - g_waveforms.at(ihp)->GetYaxis()->GetXmin();
		for(auto marker = fTDCMarkers[ihp].begin(); marker != fTDCMarkers[ihp].end(); ++marker){
			marker->SetY(y);
			marker->Draw();
		}
		for(auto text = fTDCMarkerText[ihp].begin(); text != fTDCMarkerText[ihp].end(); ++text){
			text->SetY(y + 0.05 * yRange);
			text->Draw();
		}
		fADCSumText[ihp].first.Draw();
		fADCSumText[ihp].second.Draw();
                gPad->Update();
        }
	else std::cerr<<"Can not find waveform graph!!! channel = "<<ihp<<std::endl;
        gPad = store;
}
