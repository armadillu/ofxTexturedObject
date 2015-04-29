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
		map<TexturedObject*, vector<TexturedObject::TexturedObjectSizeUnit>* >::iterator it;
		it = objectTexturesData.begin();
		while( it != objectTexturesData.end()){

			vector<TexturedObject::TexturedObjectSizeUnit>* textures = it->second;

			for(int i = 0; i < textures->size(); i++){

				TexturedObject::TexturedObjectSizeUnit & imageSizes = textures->at(i);

				map<TexturedObjectSize, TexturedObject::TextureUnit>::iterator it2 = imageSizes.sizes.begin();
				while( it2 != imageSizes.sizes.end() ){

					TexturedObjectSize imgSize = it2->first;

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

		//not all the time TODO!
		if(ofGetFrameNum()%30 == 0){
			update();
		}
		
		string msg;
		uint64_t usedBytes = pixelsInGPU * 3 /*rgb*/ * 1.333f /*mipmaps*/;

		msg = "loaded Textures (retainCount): " + ofToString(loadedTexturesCount) +
		"\nloaded Textures (ofTexture*): " + ofToString(loadedTextureRealCount);
		msg += "\nUsed Vram: " + ofxSimpleHttp::bytesToHumanReadable(usedBytes, 2) + "\n";

		for(int i = 0; i < TEXTURE_OBJECT_NUM_SIZES; i++){
			msg += toString((TexturedObjectSize)i) + ": " + ofToString(countBySize[(TexturedObjectSize)i]) + "\n";
		}


		ofDrawBitmapStringHighlight(msg, x, y);
	}

private:

	map<TexturedObject*, vector<TexturedObject::TexturedObjectSizeUnit>* > objectTexturesData;

	int loadedTexturesCount;
	int loadedTextureRealCount;
	int countBySize[TEXTURE_OBJECT_NUM_SIZES];

	uint64_t pixelsInGPU;

	ofMutex mutex;
};

#endif
