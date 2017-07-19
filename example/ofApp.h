#pragma once

#include "ofMain.h"
#include "CustomApp.h"

#include "ofxMemoryUsage.h"
#include "ofxTimeMeasurements.h"
#include "ofxRemoteUIServer.h"

#include "MyTexturedObject.h"
#include "MyScreenObject.h"
#include "ofxHistoryPlot.h"
#include "ofxRemoteUIServer.h"

#include "TexturedObjectSizes.h"

class ofApp : public CustomApp{

public:

	void setup();
	void update();
	void draw();

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

	ofxMemoryUsage mem;
	ofxHistoryPlot * memPlot;

	int resizeQuality; // CV_INTER_NN, CV_INTER_LINEAR, CV_INTER_CUBIC*, CV_INTER_AREA, CV_INTER_LANCZOS4


};
