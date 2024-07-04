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

//	mainFrame->RemoveFrame(processButton);
//	delete processButton;
	mainFrame->Cleanup();

	// Call SetupDisplay to create new elements
	SetupDisplay();
}

void EventDisplay::ErrorMessage(std::string message){
	std::cout<<"Error message: "<<message<<std::endl;
	error_label->SetText(message.c_str());
	error_label->SetTextColor(0xFF0000);
	mainFrame->Layout();
}

void EventDisplay::SetupDisplay() {
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

	TGHorizontalFrame* bottomFrame= new TGHorizontalFrame(mainFrame, 800, 30);
	// Add first TGComboBox
	boxIsOddEven = new TGComboBox(bottomFrame, "boxIsOddEven");
	boxIsOddEven->AddEntry("all hits", 0);
	boxIsOddEven->AddEntry("odd/even layers", 1);
	bottomFrame->AddFrame(boxIsOddEven, new TGLayoutHints(kLHintsBottom | kLHintsLeft, 5, 5, 10, 10));
	boxIsOddEven->Select(0);
	boxIsOddEven->Resize(150, 20);

	// Add additional TGNumberEntry
	TGLabel* entryLabel = new TGLabel(bottomFrame, "Entry");
	bottomFrame->AddFrame(entryLabel, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 0, 0, 10, 10));
	eventEntry = new TGNumberEntry(bottomFrame, 0, 9, -1, TGNumberFormat::kNESInteger);
	bottomFrame->AddFrame(eventEntry, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));

	// Add second TGComboBox
	boxZPos = new TGComboBox(bottomFrame, "boxZPos");
	boxZPos->AddEntry("RO", 0);
	boxZPos->AddEntry("Z0", 1);
	boxZPos->AddEntry("HV", 2);
	bottomFrame->AddFrame(boxZPos, new TGLayoutHints(kLHintsBottom | kLHintsRight, 5, 5, 10, 10));
	boxZPos->Select(0);
	boxZPos->Resize(150, 20);

	mainFrame->AddFrame(bottomFrame, new TGLayoutHints(kLHintsBottom | kLHintsCenterX, 5, 5, 10, 10));

	//Connections
	boxIsOddEven->Connect("Selected(Int_t)",                 "EventDisplay", this, "IsOddEvenSelected(int)");
	eventEntry->Connect("ValueSet(Long_t)",                  "EventDisplay", this, "EntryNumberChanged()");
	boxZPos->Connect("Selected(Int_t)",                      "EventDisplay", this, "ZPosSelected(int)");
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

void EventDisplay::UpdateWaveforms(){
	std::cout<<"Update waveform graphs"<<std::endl;
	//release memory of previous waveforms
	for(auto waveform = g_waveforms.begin(); waveform != g_waveforms.end(); ++waveform){
		delete waveform->second;
		waveform->second = nullptr;
	}
	fTDCMarkers.clear();
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
			TMarker m((double)(*tdc) / 32, 0, 22);
//			std::cout<<"TDC marker at x = "<<(double)(*tdc) / 32<<std::endl;
			m.SetMarkerColor(kRed);
			m.SetMarkerSize(1);
			fTDCMarkers[channel].push_back(m);
		}
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
		canvas->SetCanvasSize(800, 600);
		TH1F* frame = gPad->DrawFrame(-850., -850., 850., 850.);
		frame->SetTitle(Form("EventDisplay run%d entry %d", fRunNum, fEvent));
		frame->GetXaxis()->SetTitle("X [mm]");
		frame->GetYaxis()->SetTitle("Y [mm]");
		DrawCDCXY();

		graph = new TGraph();
		graph->SetMarkerStyle(20);
		graph->SetMarkerSize(0.4);
		graph->SetMarkerColor(1);
		for(std::vector<CDCHit*>::const_iterator hit = fHits->begin(); hit != fHits->end(); ++hit){
			int channel = (*hit)->GetChannelID();
			TVector2 pos;
			if(fZPosEntry == 0) pos = CDCGeom::Get().ChannelToROPos(channel);
			else if(fZPosEntry == 1) pos = CDCGeom::Get().ChannelToZ0Pos(channel);
			else if(fZPosEntry == 2) pos = CDCGeom::Get().ChannelToHVPos(channel);
			else{ std::cerr<<"Error of z position entry!!!"<<std::endl; exit(-1); }
			graph->SetPoint(channel, pos.X(), pos.Y());
		}
		for(int i=0; i<graph->GetN(); ++i){
			double x,y;
			graph->GetPoint(i, x, y);
			if(x == 0 && y == 0) graph->SetPoint(i, 99999, 99999);
		}
		graph->Draw("P");
		gPad->Update();
		graph->SetHighlight();
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
		canvas->SetCanvasSize(800, 400);
		canvas->Divide(2,1);
		graph_odd = new TGraph();
		graph_odd->SetMarkerStyle(20);
		graph_odd->SetMarkerSize(0.4);
		graph_odd->SetMarkerColor(1);
		graph_even = new TGraph();
		graph_even->SetMarkerStyle(20);
		graph_even->SetMarkerSize(0.4);
		graph_even->SetMarkerColor(1);
		for(std::vector<CDCHit*>::const_iterator hit = fHits->begin(); hit != fHits->end(); ++hit){
			int channel = (*hit)->GetChannelID();
			int layer = CDCGeom::Get().ChannelToLayer(channel);
			TVector2 pos;
			if(fZPosEntry == 0) pos = CDCGeom::Get().ChannelToROPos(channel);
			else if(fZPosEntry == 1) pos = CDCGeom::Get().ChannelToZ0Pos(channel);
			else if(fZPosEntry == 2) pos = CDCGeom::Get().ChannelToHVPos(channel);
			else{ std::cerr<<"Error of z position entry!!!"<<std::endl; exit(-1); }

			if(layer % 2 == 1) graph_odd->SetPoint(channel, pos.X(), pos.Y());
			else graph_even->SetPoint(channel, pos.X(), pos.Y());
		}
		for(int i=0; i<graph_odd->GetN(); ++i){
			double x,y;
			graph_odd->GetPoint(i, x, y);
			if(x == 0 && y == 0) graph_odd->SetPoint(i, 99999, 99999);
		}
		for(int i=0; i<graph_even->GetN(); ++i){
			double x,y;
			graph_even->GetPoint(i, x, y);
			if(x == 0 && y == 0) graph_even->SetPoint(i, 99999, 99999);
		}
		canvas->cd(1);
		TH1F* frame_odd = gPad->DrawFrame(-850., -850., 850., 850.);
		frame_odd->SetTitle(Form("EventDisplay for odd layers at run%d entry %d", fRunNum, fEvent));
		frame_odd->GetXaxis()->SetTitle("X [mm]");
		frame_odd->GetYaxis()->SetTitle("Y [mm]");
		DrawCDCXY();
		graph_odd->Draw("P");
		gPad->Update();
		graph_odd->SetHighlight();
		canvas->cd(2);
		TH1F* frame_even = gPad->DrawFrame(-850., -850., 850., 850.);
		frame_even->SetTitle(Form("EventDisplay for even layers at run%d entry %d", fRunNum, fEvent));
		frame_even->GetXaxis()->SetTitle("X [mm]");
		frame_even->GetYaxis()->SetTitle("Y [mm]");
		DrawCDCXY();
		graph_even->Draw("P");
		gPad->Update();
		graph_even->SetHighlight();
	}

	gPad = store;
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
                g_waveforms.at(ihp)->Draw("APL");
		for(auto marker = fTDCMarkers[ihp].begin(); marker != fTDCMarkers[ihp].end(); ++marker){
			marker->SetY(g_waveforms.at(ihp)->GetYaxis()->GetXmin());
			marker->Draw();
		}
                gPad->Update();
        }
	else std::cerr<<"Can not find waveform graph!!!"<<std::endl;
        gPad = store;
}
