//
//  MyScreenObject.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 18/07/2017.
//
//

#include "MyScreenObject.h"
#include "MyTexturedObject.h"

MyScreenObject::MyScreenObject(){

}


void MyScreenObject::setup(MyTexturedObject * texObj, ofRectangle * loadArea, float objScale){

	this->texObj = texObj;
	this->loadArea = loadArea;

	vel = ofVec2f(ofRandom(0.5, 6), ofRandom(-1, 1));

	me.width = texObj->getTextureDimensions(TEXTURE_ORIGINAL, 0).x * objScale;
	me.height = texObj->getTextureDimensions(TEXTURE_ORIGINAL, 0).y * objScale;
	me.x = ofRandom(0, ofGetWidth() - me.width);
	me.y = ofRandom(0, ofGetHeight() - BOTTOM_GAP - me.height);

	tex = texObj->getRealTexture(TEXTURE_ORIGINAL, 0); //get hold of the real ofTexure*

}


void MyScreenObject::update(float dt, float speedGain, bool useMipmaps){

	withMipmaps = useMipmaps;

	me.x = me.x + vel.x * dt * speedGain;
	me.y = me.y + vel.y * dt * speedGain;

	if(me.x > ofGetWidth()) me.x = 0;
	if(me.y > ofGetHeight() - BOTTOM_GAP) me.y = 0;
	if(me.y < 0) me.y = ofGetHeight() - BOTTOM_GAP;

	bool onScreenNow = loadArea->intersects(me);

	if(onScreenNow != onScreen){
		if(onScreenNow){ //we jus got on screen
			requestTex();
		}else{ //we just left screen
			releaseTex();
		}
	}
	onScreen = onScreenNow;

}


void MyScreenObject::draw(){

	ofSetColor(255);

	if((texLoaded || texReadyToDraw)){
		//this will smartly return the right texture, when the intended texture is loaded it returns that
		//otherwise it retunrs a "loading" texture of your chosing, an error texture, or whatever suits the current state.
		ofTexture * smartTexture = texObj->getTexture(TEXTURE_ORIGINAL, 0);
		ofTexture * realTexture = texObj->getRealTexture(TEXTURE_ORIGINAL, 0);
		ofRectangle mySize = me;
		ofRectangle texSize = ofRectangle(0,0, realTexture->getWidth(), realTexture->getHeight());
		texSize.scaleTo(mySize);
		realTexture->draw(texSize);

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
	glLineWidth(2);
	ofDrawRectangle(me);
	glLineWidth(1);
	ofFill();

	if(ofGetKeyPressed('t')){
		string msg = "Retain Count: " + ofToString(texObj->getRetainCount(TEXTURE_ORIGINAL, 0));
		msg += "\nWaitingForTexture: " + string(waitingForTex ? "TRUE" : "FALSE");
		msg += "\nTexture Ready To Draw: " + string(texReadyToDraw ? "TRUE" : "FALSE");
		msg += "\nTexture Fully loaded: " + string(texLoaded ? "TRUE" : "FALSE");
		if(texLoaded) msg += "\nLoadTime: " + ofToString(timeToLoad, 1);
		ofDrawBitmapStringHighlight(msg, me.x, me.y + me.height);
	}
}


void MyScreenObject::setPosition(float x, float y){
	me.x = x;
	me.y = y;
}


//get notified when texture is ready to draw
void MyScreenObject::onTexReadyToDraw(TexturedObject::TextureEventArg & a){
	if(a.tex == tex && waitingForTex){ //note this event may trigger for other objects that request this text, so we check if we are "waitingForTex"
		ofRemoveListener(texObj->textureReadyToDraw, this, &MyScreenObject::onTexReadyToDraw);
		texReadyToDraw = true;
	}
}


//get notified when texture is fully loaded
void MyScreenObject::onTexLoaded(TexturedObject::TextureEventArg & a){
	if(a.tex == tex && waitingForTex){ //note this event may trigger for other objects that request this text, so we check if we are "waitingForTex"
		ofRemoveListener(texObj->textureLoaded, this, &MyScreenObject::onTexLoaded);
		waitingForTex = false;
		texLoaded = true;
		timeToLoad = a.elapsedTime;
	}
}



void MyScreenObject::requestTex(){

	//ofLogNotice("MyScreenObject") << ofGetFrameNum() << " - request texture " << this;
	texLoaded = false;
	texReadyToDraw = false;
	waitingForTex = true;
	ofAddListener(texObj->textureReadyToDraw, this, &MyScreenObject::onTexReadyToDraw);
	ofAddListener(texObj->textureLoaded, this, &MyScreenObject::onTexLoaded);
	tex = texObj->requestTexture(	TEXTURE_ORIGINAL, //tex size
						 			0, //tex index
						 			ofGetKeyPressed('1'), //highPriority
								 	withMipmaps
						 		);
}


void MyScreenObject::releaseTex(){

	//ofLogNotice("MyScreenObject") << ofGetFrameNum() << " - release texture " << this;
	if(waitingForTex){
		ofRemoveListener(texObj->textureLoaded, this, &MyScreenObject::onTexLoaded);
		if(!texReadyToDraw) ofRemoveListener(texObj->textureReadyToDraw, this, &MyScreenObject::onTexReadyToDraw);
	}
	texObj->releaseTexture(TEXTURE_ORIGINAL, 0);
	waitingForTex = false;
	texLoaded = false;
	texReadyToDraw = false;
}
