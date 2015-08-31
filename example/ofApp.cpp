#include "ofApp.h"
#include "TexturedObject.h"
#include "TexturedObjectStats.h"

int numObj = 100;
float objectSize = 50;


void ofApp::setup(){

	CustomApp::setup();

	// LISTENERS
	ofAddListener(screenSetup.setupChanged, this, &ofApp::setupChanged);

	ofEnableArbTex();
	ofDisableArbTex();

	screen = new ofRectangle(0,0,0,0);

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
	q->setNumberSimultaneousLoads( 8 ); //N threads loading images in the bg
	q->setTexLodBias(0.2); //MipMap sharpness

	// Setup our TexturedObjects /////////////////////////////////

	ifstream infile;
	char cNum[1024] ; //load image list
	infile.open (ofToDataPath("images.sorted.txt").c_str(), ifstream::in);
	if (infile.is_open()){
		while (infile.good()){
			infile.getline(cNum, 1024, '\n');
			imagePaths.push_back(string(cNum));
		}
		infile.close();
	}

	//create TextureObjects
	for(int i = 0; i < numObj; i++){
		MyTexturedObject* o = new MyTexturedObject();
		o->setup(imagePaths[i%(imagePaths.size())], screen, objectSize);
		objs.push_back(o);
	}



	//start ofxRemoteUI Server
	RUI_SETUP();

	windowSize = 400;
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
	RUI_SHARE_PARAM(windowSize, 10, 1200);

	RUI_LOAD_FROM_XML();

	TIME_SAMPLE_SET_FRAMERATE(60);
	TIME_SAMPLE_ENABLE();
	TIME_SAMPLE_SET_DRAW_LOCATION(TIME_MEASUREMENTS_TOP_RIGHT);


}


void ofApp::update(){

	mem.update();
	if(ofGetFrameNum()%3 == 1){
		memPlot->update(mem.getProcessMemory());
	}

	if(ofGetFrameNum() > 10){
		screen->x = ofGetMouseX();
		screen->y = ofGetMouseY();
		screen->width = windowSize;
		screen->height = windowSize;
	}

	//UPDATE ProgressiveTextureLoadQueue settings
	ProgressiveTextureLoadQueue * q = ProgressiveTextureLoadQueue::instance();
	q->setTargetTimePerFrame( texLoadTimePerFrame );	//spend at most 'x' milis loading textures per frame
	q->setScanlinesPerLoop(scanlinesPerLoop);
	q->setMaximumRequestsPerFrame(maxTexRequestsPerFrame);
	q->setNumberSimultaneousLoads( maxThreads ); //N threads loading images in the bg

	if(resetAll){
		for(int i = 0; i < numObj; i++){
			objs[i]->pos.x = ofRandom(ofGetWidth());
			objs[i]->pos.y = ofRandom(ofGetHeight() - 100);
		}
		resetAll = false;
		RUI_PUSH_TO_CLIENT();
	}

	for(int i = 0; i < numObj; i++){
		objs[i]->update(speedFactor, texUnloadDelay, loadMipMaps);
		objs[i]->setResizeQuality(resizeQuality);
	}
}


void ofApp::draw(){

	ofSetColor(255);
	ofNoFill();
	ofRect(*screen);
	ofFill();

	for(int i = 0; i < numObj; i++){
		objs[i]->draw();
	}

	int y = 40;
	int x = 20;

	TexturedObjectStats::one().draw(x, y);
	y += 16 * 8 + 4;

	ProgressiveTextureLoadQueue::instance()->draw(x, y);
	y += 52 + ProgressiveTextureLoadQueue::instance()->getNumBusy() * 15;

	//see avg load times
	float avgTime = 0;
	int numLoaded = 0;
	for(int i = 0; i < numObj; i++){
		if(objs[i]->texLoaded){
			avgTime += objs[i]->timeToLoad;
		}
	}

	ofDrawBitmapStringHighlight("avg loadTime: " + ofToString(avgTime, 2), 20, ofGetHeight() - 160);

	int dh = 120;
	memPlot->draw(0, ofGetHeight() - dh, ofGetWidth(), dh);
}


void ofApp::keyPressed(int key){

	if(key == 'w'){
		screenSetup.cycleToNextScreenMode();
	}
}


//////// CALLBACKS //////////////////////////////////////

void ofApp::setupChanged(ofxScreenSetup::ScreenSetupArg &arg){
	ofLogNotice()	<< "ofxScreenSetup setup changed from " << screenSetup.stringForMode(arg.oldMode)
	<< " (" << arg.oldWidth << "x" << arg.oldHeight << ") "
	<< " to " << screenSetup.stringForMode(arg.newMode)
	<< " (" << arg.newWidth << "x" << arg.newHeight << ")";
}
