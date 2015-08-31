//
//  MyTexturedObject.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 17/12/14.
//
//

#ifndef BaseApp_MyTexturedObject_h
#define BaseApp_MyTexturedObject_h

#include "ofMain.h"
#include "TexturedObject.h"

#define BOTTOM_GAP		200

class MyTexturedObject : public TexturedObject{

public:

	void setup(string imgPath_, ofRectangle * screen_, float objSize){

		//imgPaths = imgPaths_;
		vel = ofVec2f( ofRandom(0.5, 6), ofRandom(-1, 1));
		pos.x = ofRandom(0, ofGetWidth() - objSize);
		pos.y = ofRandom(0, ofGetHeight() - BOTTOM_GAP);
		screen = screen_;
		imgPath = imgPath_;
		TexturedObject::setup(1, TEXTURE_ORIGINAL);
		tex = getRealTexture(TEXTURE_ORIGINAL, 0);
		me.x = pos.x;
		me.y = pos.y;
		me.width = objSize;
		me.height = objSize;
		onScreen = false;
		waitingForTex = false;
		texReadyToDraw = false;
		texLoaded = false;

		//this is important in terms of memory usage - resizing large images to create mipmaps
		//takes a lot of memory - especially in 32 bit mode you can quickly run out of RAM
		setResizeQuality(CV_INTER_AREA);
	}

	void update(float speedFactor, float unloadDelay_, bool withMipmaps){

		TexturedObject::update();

		mipmaps = withMipmaps;
		unloadDelay = unloadDelay_;
		pos = pos + vel * 0.016 * speedFactor;

		if(pos.x > ofGetWidth()) pos.x = 0;
		if(pos.y > ofGetHeight() - BOTTOM_GAP) pos.y = 0;
		if(pos.y < 0) pos.y = ofGetHeight() - BOTTOM_GAP;

		me.x = pos.x;
		me.y = pos.y;

		bool onScreenNow = screen->intersects(me);

		if(onScreenNow != onScreen){
			if(onScreenNow){ //we jus got on screen
				requestTex();
			}else{ //we just left screen
				releaseTex();
			}
		}
		onScreen = onScreenNow;
	}

	void draw(){

		ofSetColor(255);
		if((texLoaded || texReadyToDraw)){
			getTexture(TEXTURE_ORIGINAL, 0)->draw(me);
		}

		if(onScreen){
			if(waitingForTex){
				if(texReadyToDraw){
					ofSetColor(255,0,255); //drawable, but not fully loaded
				}else{
					ofSetColor(255,0,0); //not fully loaded, not drawable (waiting)
				}
			}else{
				ofSetColor(0,255,0); //fully loaded
			}

		}else{
			ofSetColor(64);
		}

		ofNoFill();
		glLineWidth(4);
		ofRect(me);
		glLineWidth(1);
		ofFill();

		if(ofGetKeyPressed('t')){
			string msg = "Retain Count: " + ofToString(getRetainCount(TEXTURE_ORIGINAL, 0));
			msg += "\nWaitingForTexture: " + string(waitingForTex ? "TRUE" : "FALSE");
			msg += "\nTexture Ready To Draw: " + string(texReadyToDraw ? "TRUE" : "FALSE");
			msg += "\nTexture Fully loaded: " + string(texLoaded ? "TRUE" : "FALSE");
			if(texLoaded) msg += "\nLoadTime: " + ofToString(timeToLoad, 1);
			ofDrawBitmapStringHighlight(msg, pos.x, pos.y);
		}
	}

	//if you happen to know the draw size of your textures before hand (b4 loading it), then you can supply it here
	virtual ofVec2f getTextureDimensions(TexturedObjectSize s, int index){
		return ofVec2f(objectSize,objectSize);
	}

	//supply a size and texture index, get its local file path
	virtual string getLocalTexturePath(TexturedObjectSize, int index){
		return imgPath;
	}

	//NEVER delete a TexturedObject directly!
	virtual void deleteWithGC(){
		TexturedObject::deleteWithGC();
	}



	//get notified when texture is ready to draw
	void onTexReadyToDraw(TexturedObject::TextureEventArg & a){
		//LOG;
		if(a.tex == tex && waitingForTex){
			ofLogNotice() << "tex ready to draw " << tex;
			ofRemoveListener(textureReadyToDraw, this, &MyTexturedObject::onTexReadyToDraw);
			texReadyToDraw = true;
		}
	}

	//get notified when texture is fully loaded
	void onTexLoaded(TexturedObject::TextureEventArg & a){
		//LOG;
		if(a.tex == tex && waitingForTex){
			ofLogNotice() << "tex loaded " << tex;
			ofRemoveListener(textureLoaded, this, &MyTexturedObject::onTexLoaded);
			waitingForTex = false;
			texLoaded = true;
			timeToLoad = a.elapsedTime;
		}
	}


	//ask the texture to load
	void requestTex(){
		//LOG;
		texLoaded = false;
		texReadyToDraw = false;
		waitingForTex = true;
		ofAddListener(textureReadyToDraw, this, &MyTexturedObject::onTexReadyToDraw);
		ofAddListener(textureLoaded, this, &MyTexturedObject::onTexLoaded);
		tex = requestTexture(TEXTURE_ORIGINAL, //tex size
							 0, //tex index
							 ofGetKeyPressed('1'), //highPriority
							 mipmaps //mipmaps
							 );
	}

	//release the texture
	void releaseTex(){
		//LOG;
		if(waitingForTex){
			ofRemoveListener(textureLoaded, this, &MyTexturedObject::onTexLoaded);
		}
		releaseTexture(TEXTURE_ORIGINAL, 0, unloadDelay/*delay*/);
		waitingForTex = false;
		texLoaded = false;
		texReadyToDraw = false;
		//imgPath = imgPaths[(int)ofRandom(imgPaths.size()-1)];
	}

	ofVec2f pos;
	ofVec2f vel;

	float timeToLoad;

	bool waitingForTex;
	bool texReadyToDraw;
	bool texLoaded;

private:

	ofRectangle* screen;
	ofRectangle me;

	string imgPath;
	//vector<string> imgPaths;
	ofTexture * tex;
	float unloadDelay;


	bool mipmaps;
	bool onScreen;

	float objectSize;

};

#endif
