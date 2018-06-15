#pragma once

#define CHECK_MEM_USE false

#include "ofMain.h"
#include "CustomApp.h"

#if CHECK_MEM_USE
	#include "ofxMemoryUsage.h"
#endif

#include "ofxTimeMeasurements.h"
#include "ofxRemoteUIServer.h"

#include "MyTexturedObject.h"
#include "MyScreenObject.h"
#include "ofxHistoryPlot.h"
#include "ofxRemoteUIServer.h"

class ofApp : public CustomApp{

public:

	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);

	// APP CALLBACKS ////////////////////////////////////////

	void setupChanged(ofxScreenSetup::ScreenSetupArg &arg);
	void remoteUIClientDidSomething(RemoteUIServerCallBackArg & arg);


	// APP SETUP ////////////////////////////////////////////

	vector<string> imagePaths;
	vector<MyTexturedObject*> textureObjects;
	vector<MyScreenObject*> screenObjects;

	ofRectangle * screen;

	ofTexture * loading;
	ofTexture * cancel;
	ofTexture * error;
	ofTexture * missing;

	float speedFactor;
	float texLoadTimePerFrame;
	float texUnloadDelay;
	bool resetAll;
	bool loadMipMaps;
	float windowSizeY;
	float windowSizeX;

	int scanlinesPerLoop;
	int maxTexRequestsPerFrame;
	int maxThreads;

	#if CHECK_MEM_USE
	ofxMemoryUsage mem;
	ofxHistoryPlot * memPlot;
	#endif

	int resizeQuality; // CV_INTER_NN, CV_INTER_LINEAR, CV_INTER_CUBIC*, CV_INTER_AREA, CV_INTER_LANCZOS4


};
