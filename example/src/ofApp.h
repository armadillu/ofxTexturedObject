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

	float speedFactor = 10.0;
	float texLoadTimePerFrame = 0.1f;
	float texUnloadDelay = 0.0f;
	bool resetAll;
	bool loadMipMaps = true;
	float windowSizeY = 400;
	float windowSizeX = 400;

	int scanlinesPerLoop = 4;
	int maxTexRequestsPerFrame = 100;
	int maxThreads = 4;

	#if CHECK_MEM_USE
	ofxMemoryUsage mem;
	ofxHistoryPlot * memPlot;
	#endif

	int resizeQuality = CV_INTER_AREA; // CV_INTER_NN, CV_INTER_LINEAR, CV_INTER_CUBIC*, CV_INTER_AREA, CV_INTER_LANCZOS4


};
