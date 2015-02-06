//
//  TexturedObjectStats.h
//  CollectionTable
//
//  Created by Oriol Ferrer Mesià on 11/10/14.
//
//

#ifndef CollectionTable_TexturedObjectStats_h
#define CollectionTable_TexturedObjectStats_h

#include "ofMain.h"
#include "TexturedObject.h"
#include "ofxSimpleHttp.h"
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
		map<TexturedObject*, vector<TexturedObject::ImageSizeUnit>* >::iterator it;
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

		//this assume we are only using RIVER_IMAGE_SIZE and ZOOMABLE_IMAGE_SIZE! todo!
		for(int j = 0 ; j < IMG_NUM_SIZES; j++){
			countBySize[j] = 0;
		}

		mutex.lock();
		map<TexturedObject*, vector<TexturedObject::ImageSizeUnit>* >::iterator it;
		it = objectTexturesData.begin();
		while( it != objectTexturesData.end()){

			vector<TexturedObject::ImageSizeUnit>* textures = it->second;

			for(int i = 0; i < textures->size(); i++){

				TexturedObject::ImageSizeUnit & imageSizes = textures->at(i);

				map<ImageSize, TexturedObject::ImageUnit>::iterator it2 = imageSizes.sizes.begin();
				while( it2 != imageSizes.sizes.end() ){

					ImageSize imgSize = it2->first;

					if(it2->second.loadCount > 0){
						loadedTexturesCount ++;
						ofTexture * tex = it2->second.texture;
						if(tex->getWidth() > 0) loadedTextureRealCount++;
						pixelsInGPU += tex->getWidth() * tex->getHeight();
						countBySize[imgSize] ++;
					}
					++it2;
				}
			}
			++it;
		}
		mutex.unlock();
		TS_STOP("Texture Stats");
	}

	void draw(int x, int y){

		//if(ofGetFrameNum()%30 == 1){
			update();
		//}
		
		string msg;
		uint64_t usedBytes = pixelsInGPU * 3 /*rgb*/ * 1.333f /*mipmaps*/;

		msg = "loaded Textures (retainCount): " + ofToString(loadedTexturesCount)  +
		"\nloaded Textures (ofTexture*): " + ofToString(loadedTextureRealCount)  +
		"\n(" + ofToString(countBySize[RIVER_IMAGE_SIZE])
		+ " small, " +
		ofToString(countBySize[ZOOMABLE_IMAGE_SIZE]) + " big) " +
		"Used Vram: " + ofToString(ofxSimpleHttp::bytesToHumanReadable(usedBytes,2));
		ofDrawBitmapStringHighlight(msg, x, y);
	}

private:

	map<TexturedObject*, vector<TexturedObject::ImageSizeUnit>* > objectTexturesData;

	int loadedTexturesCount;
	int loadedTextureRealCount;
	int countBySize[IMG_NUM_SIZES];

	uint64_t pixelsInGPU;

	ofMutex mutex;
};

#endif
