#include "ofApp.h"
#include "TexturedObject.h"
#include "TexturedObjectStats.h"

int numObj = 600;
float objSize = 64; //pix


void ofApp::setup(){

	CustomApp::setup();

	// LISTENERS
	ofAddListener(screenSetup.setupChanged, this, &ofApp::setupChanged);

	ofEnableArbTex();
	ofDisableArbTex();

	screen = new ofRectangle(0,0,0,0);

	#if CHECK_MEM_USE
	memPlot = new ofxHistoryPlot(NULL, "RAM", 1000, false);
	memPlot->setLowerRange(0);
	memPlot->setColor( ofColor(0,255,0) ); //color of the plot line
	memPlot->setShowNumericalInfo(true);  //show the current value and the scale in the plot
	memPlot->setRespectBorders(true);	   //dont let the plot draw on top of text
	memPlot->setLineWidth(1);				//plot line width
	memPlot->setBackgroundColor(ofColor(0,220)); //custom bg color
	//custom grid setup
	memPlot->setDrawGrid(true);
	memPlot->setGridColor(ofColor(30)); //grid lines color
	memPlot->setGridUnit(14);
	#endif

	// Setup TexturedObjectConfig /////////////////////////

	loading = new ofTexture();
	ofLoadImage(*loading, "TexturedObjectStates/loading.png");
	cancel = new ofTexture();
	ofLoadImage(*cancel, "TexturedObjectStates/canceling.png");
	error = new ofTexture();
	ofLoadImage(*error, "TexturedObjectStates/error.png");
	missing = new ofTexture();
	ofLoadImage(*missing, "TexturedObjectStates/missing.png");

	vector<ofTexture*> loadingTexs;
	loadingTexs.push_back(loading);
	TexturedObjectConfig::one().setTextures(missing, error, cancel, loadingTexs);

	// Setup ProgressiveTextureLoadQueue /////////////////////////

	ProgressiveTextureLoadQueue * q = ProgressiveTextureLoadQueue::instance();
	q->setVerbose(false);

	// Setup our TexturedObjects /////////////////////////////////

	ofDirectory dir;
	dir.allowExt("jpeg");
	dir.allowExt("jpg");
	dir.allowExt("png");

	dir.listDir("images");
	int n = dir.numFiles();
	for(int i = 0; i < n; i++){
		imagePaths.push_back(dir.getPath(i));
	}

	//create TextureObjects
	for(auto & path : imagePaths){
		MyTexturedObject* o = new MyTexturedObject();
		o->setup(path);
		textureObjects.push_back(o);
	}

	//create Screen Objects
	for(int i = 0; i < numObj; i++){
		MyScreenObject* o = new MyScreenObject();
		auto randomTexObject = textureObjects[i%(textureObjects.size())];
		o->setup(randomTexObject, screen, objSize);
		screenObjects.push_back(o);
	}

	//start ofxRemoteUI Server
	RUI_SETUP();

	windowSizeX = windowSizeY = 400;
	maxTexRequestsPerFrame = 20;
	RUI_NEW_GROUP("TEXTURE LOADER");
	RUI_SHARE_PARAM(texLoadTimePerFrame, 0.001, 10);
	RUI_SHARE_PARAM(texUnloadDelay, 0.0, 10);
	RUI_SHARE_PARAM(scanlinesPerLoop, 1, 512);
	RUI_SHARE_PARAM(maxTexRequestsPerFrame, 1, 200);
	RUI_NEW_COLOR();
	RUI_SHARE_PARAM(maxThreads, 1, 40);

	RUI_SHARE_PARAM(loadMipMaps);
	vector<string> list;
	list.push_back("CV_INTER_NN");
	list.push_back("CV_INTER_LINEAR");
	list.push_back("CV_INTER_CUBIC");
	list.push_back("CV_INTER_AREA");
	list.push_back("CV_INTER_LANCZOS4");
	resizeQuality = CV_INTER_NN;
	RUI_SHARE_ENUM_PARAM(resizeQuality, CV_INTER_NN, CV_INTER_LANCZOS4, list );

	RUI_NEW_GROUP("APP");

	RUI_SHARE_PARAM(speedFactor, 0.00, 500);
	RUI_SHARE_PARAM(resetAll);
	RUI_SHARE_PARAM(windowSizeX, 10, 1200);
	RUI_SHARE_PARAM(windowSizeY, 10, 1200);

	RUI_LOAD_FROM_XML();

	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_ENABLE();
	TIME_SAMPLE_SET_DRAW_LOCATION(TIME_MEASUREMENTS_TOP_RIGHT);
}


void ofApp::update(){

	TSGL_START("gl u");

	float dt = 0.016666;
	#if CHECK_MEM_USE
	mem.update();
	if(ofGetFrameNum()%3 == 1){
		memPlot->update(mem.getProcessMemory());
	}
	#endif

	screen->x = ofGetMouseX();
	screen->y = ofGetMouseY();
	screen->width = windowSizeX;
	screen->height = windowSizeY;

	//UPDATE ProgressiveTextureLoadQueue settings
	ProgressiveTextureLoadQueue * q = ProgressiveTextureLoadQueue::instance();
	q->setTargetTimePerFrame( texLoadTimePerFrame );	//spend at most 'x' milis loading textures per frame
	q->setScanlinesPerLoop(scanlinesPerLoop);
	q->setMaximumRequestsPerFrame(maxTexRequestsPerFrame);
	q->setMaxThreads( maxThreads ); //N threads loading images in the bg
	TexturedObjectConfig::one().setDefaultTextureUnloadDelay(texUnloadDelay);

	if(resetAll){
		for(auto s : screenObjects){
			s->setPosition(ofRandom(ofGetWidth()), ofRandom(ofGetHeight() - 200) );
		}
		resetAll = false;
		RUI_PUSH_TO_CLIENT();
	}

	TS_START("texObjects");
	float timeNow = ofGetElapsedTimef();
	for(auto s : textureObjects){
		s->update(timeNow);
	}
	TS_STOP("texObjects");

	TS_START("screenObjects");
	for(auto s : screenObjects){
		s->update(dt, speedFactor, loadMipMaps);
	}
	TS_STOP("screenObjects");

	TSGL_STOP("gl u");
}


void ofApp::draw(){

	ofSetColor(255);
	ofNoFill();
	ofDrawRectangle(*screen);
	ofFill();

	for(auto s : screenObjects){
		s->draw();
	}

	int y = 40;
	int x = 20;

	TexturedObjectStats::one().draw(x, y);
	y += 16 * 4 + 4;

	ProgressiveTextureLoadQueue::instance()->draw(x, y);
	y += 52 + ProgressiveTextureLoadQueue::instance()->getNumBusy() * 15;

	//see avg load times
	float avgTime = 0;
	int numSamples = 0;
	for(auto s : screenObjects){
		if(s->isTexLoaded()){
			avgTime += s->getLastLoadTime();
			numSamples++;
		}
	}

	if(numSamples > 0){
		ofDrawBitmapStringHighlight("avg loadTime: " + ofToString(avgTime / numSamples, 2) + " sec", 20, ofGetHeight() - 160);
	}

	#if CHECK_MEM_USE
	int dh = 120;
	memPlot->draw(0, ofGetHeight() - dh, ofGetWidth(), dh);
	#endif
}


void ofApp::keyPressed(int key){

	if(key == 'w'){
		screenSetup.cycleToNextScreenMode();
	}

	if(key == 'r'){
		resetAll = true;
	}

}

void ofApp::exit() {

	for (auto t : textureObjects) {
		if (t->isLoadingTextures()) {
			t->waitForThread();
		}
	}

}

//////// CALLBACKS //////////////////////////////////////

void ofApp::setupChanged(ofxScreenSetup::ScreenSetupArg &arg){
	ofLogNotice()	<< "ofxScreenSetup setup changed from " << screenSetup.stringForMode(arg.oldMode)
	<< " (" << arg.oldWidth << "x" << arg.oldHeight << ") "
	<< " to " << screenSetup.stringForMode(arg.newMode)
	<< " (" << arg.newWidth << "x" << arg.newHeight << ")";
}
