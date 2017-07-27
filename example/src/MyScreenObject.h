//
//  MyScreenObject.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 18/07/2017.
//
//

#pragma once

#define BOTTOM_GAP		200


#include "ofMain.h"
#include "TexturedObject.h"

class MyTexturedObject;

class MyScreenObject{

public:
	
	MyScreenObject();

	void setup(MyTexturedObject * texObj, ofRectangle * loadArea, float objSize);

	void update(float dt, float speedGain, bool useMipmaps);

	void draw();

	void requestTex();
	void releaseTex();

	//get notified when texture is ready to draw
	void onTexReadyToDraw(TexturedObject::TextureEventArg & a);
	//get notified when texture is fully loaded
	void onTexLoaded(TexturedObject::TextureEventArg & a);

	void setPosition(float x, float y);

	float getLastLoadTime(){return timeToLoad;}
	bool isTexLoaded(){return texLoaded;}

protected:

	MyTexturedObject * texObj;
	ofTexture * tex; //my tex

	ofRectangle* loadArea; //the user interactable area which makes textures load when inside the area
	ofVec2f vel;
	ofRectangle me; //my rect


	float timeToLoad;

	bool waitingForTex = false;
	bool texReadyToDraw = false;
	bool texLoaded = false;

	bool onScreen = false;

	bool withMipmaps;
	float objSize;
};

