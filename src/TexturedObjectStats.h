//
//  TexturedObjectStats.h
//  CollectionTable
//
//  Created by Oriol Ferrer Mesià on 11/10/14.
//
//

#pragma once

#include "ofMain.h"
#include "TexturedObject.h"
#include "ofxTimeMeasurements.h"

class TexturedObjectStats{

public:

	static TexturedObjectStats& one(){
		static TexturedObjectStats instance;
		// Instantiated on first use.
		return instance;
	}


	void addTextureObject(TexturedObject * obj){
		mutex.lock();
		objectTexturesData[obj] = &obj->textures;
		mutex.unlock();
	}


	void removeTextureObject(TexturedObject * obj){
		mutex.lock();
		map<TexturedObject*, vector<TexturedObject::TexturedObjectSizeUnit>* >::iterator it;
		it = objectTexturesData.find(obj);
		if (it != objectTexturesData.end() ){
			objectTexturesData.erase(it);
		}
		mutex.unlock();
	}


	void update(){

		TS_START("Texture Stats");
		pixelsInGPU = 0;
		loadedTexturesCount = 0;
		loadedTextureRealCount = 0;

		for(int j = 0 ; j < TEXTURE_OBJECT_NUM_SIZES; j++){
			countBySize[j] = 0;
		}

		mutex.lock();
		for(auto it : objectTexturesData){

			vector<TexturedObject::TexturedObjectSizeUnit>* textures = it.second;

			for(size_t i = 0; i < textures->size(); i++){

				TexturedObject::TexturedObjectSizeUnit & imageSizes = textures->at(i);

				for(auto it2 : imageSizes.sizes){

					TexturedObjectSize imgSize = it2.first;

					if(it2.second.loadCount > 0){

						loadedTexturesCount ++;
						ofTexture * tex = it2.second.texture;
						if(tex){

							if (tex->isAllocated()) {
								loadedTextureRealCount++;
							}

							int w, h;
							#ifndef TARGET_OPENGLES
							if (tex->texData.textureTarget == GL_TEXTURE_RECTANGLE_ARB) {
								w = ofNextPow2(tex->getWidth());
								h = ofNextPow2(tex->getHeight());
							} else {
								w = tex->getWidth();
								h = tex->getHeight();
							}
							#else
							w = tex->getWidth();
							h = tex->getHeight();
							#endif

							int numC = ofGetNumChannelsFromGLFormat(ofGetGLFormatFromInternal(tex->texData.glInternalFormat));
							pixelsInGPU += w * h * numC;
							if (tex->hasMipmap()) {
								pixelsInGPU *= 1.3333; //mipmaps take 33% more memory
							}

						}

						countBySize[imgSize] ++;
					}
				}
			}
		}
		mutex.unlock();
		TS_STOP("Texture Stats");
	}


	void draw(int x, int y){
		ofDrawBitmapStringHighlight(getStatsAsText(), x, y, ofColor::black, ofColor::yellow);
	}


	string getStatsAsText() {
		//not all the time TODO!
		if (ofGetFrameNum() % 30 == 0) {
			update();
		}

		string msg;
		uint64_t usedBytes = pixelsInGPU;

		msg = "loaded Textures (retainCount): " + ofToString(loadedTexturesCount) +
			"\nloaded Textures (ofTexture*): " + ofToString(loadedTextureRealCount);
		msg += "\nUsed Vram: " + bytesToHumanReadable(usedBytes, 2) + "\n";

		for (int i = 0; i < TEXTURE_OBJECT_NUM_SIZES; i++) {
			if(countBySize[(TexturedObjectSize)i] > 0){
				msg += toString((TexturedObjectSize)i) + ": " + ofToString(countBySize[(TexturedObjectSize)i]) + "\n";
			}
		}

		msg = msg.substr(0, msg.size() - 1);
		return msg;
	}


	static string bytesToHumanReadable(uint64_t bytes, int decimalPrecision){
		string ret;
		if (bytes < 1024 ){ //if in bytes range
			ret = ofToString(bytes) + " bytes";
		}else{
			if (bytes < 1024 * 1024){ //if in kb range
				ret = ofToString(bytes / float(1024), decimalPrecision) + " KB";
			}else{
				if (bytes < (1024 * 1024 * 1024)){ //if in Mb range
					ret = ofToString(bytes / float(1024 * 1024), decimalPrecision) + " MB";
				}else{
					ret = ofToString(bytes / float(1024 * 1024 * 1024), decimalPrecision) + " GB";
				}
			}
		}
		return ret;
	}


private:

	map<TexturedObject*, vector<TexturedObject::TexturedObjectSizeUnit>* > objectTexturesData;

	int loadedTexturesCount;
	int loadedTextureRealCount;
	int countBySize[TEXTURE_OBJECT_NUM_SIZES];

	uint64_t pixelsInGPU;

	ofMutex mutex;

};

