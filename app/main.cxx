#include "EventDisplay.hxx"
#include <TApplication.h>

int main(int argc, char** argv) {
	TApplication app("app", &argc, argv);
	EventDisplay* eventDisplay = new EventDisplay();
	app.Run();
	delete eventDisplay;
	return 0;
}

