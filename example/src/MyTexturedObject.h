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


class MyTexturedObject : public TexturedObject{

public:

	void setup(string imgPath_){
		imgPath = imgPath_;

		//note how we load the img pixels once through OF to find out the img dimensions
		//this is the worst but easiest option to know beforehand how large the tex will be b4 its loaded
		ofPixels pix; ofLoadImage(pix, imgPath);
		imgSize.x = pix.getWidth();
		imgSize.y = pix.getHeight();

		TexturedObject::setup(1, TEXTURE_ORIGINAL);

		//this is important in terms of memory usage - resizing large images to create mipmaps
		//takes a lot of memory - especially in 32 bit mode you can quickly run out of RAM
		TexturedObject::setResizeQuality(CV_INTER_AREA);
	}


	void update(float currentTime){
		TexturedObject::update(currentTime);
	}


	virtual ofVec2f getTextureDimensions(TexturedObjectSize s, int index){
		return imgSize;
	}


	//supply a size and texture index, get its local file path
	//note how in this example we only have one img representation per texture object, so we always return the same path
	//ignoring img index & texSize
	virtual string getLocalTexturePath(TexturedObjectSize, int index){
		return imgPath;
	}


	//NEVER delete a TexturedObject directly!
	virtual void deleteWithGC(){
		TexturedObject::deleteWithGC();
	}


private:

	string imgPath;
	ofVec2f imgSize;

};

#endif
