#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){

	//ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);

	ofGLFWWindowSettings winSettings;
	winSettings.numSamples = 0;
	int w = 1380;
	int h = 1000;
	#if OF_VERSION_MINOR < 10
	winSettings.width = w;
	winSettings.height = w;
	#else
	winSettings.setSize(w, h);
	#endif

	shared_ptr<ofAppBaseWindow> win = ofCreateWindow(winSettings);	// sets up the opengl context!
	((ofAppGLFWWindow*)win.get())->setMultiDisplayFullscreen(true);

	// 10 x 1 screens
	//ofSetupOpenGL(&win, w, h, OF_WINDOW /*OF_FULLSCREEN*/);	// <-------- setup the GL context
	ofRunApp(win, shared_ptr<ofBaseApp>(new ofApp()));
	ofRunMainLoop();
}
