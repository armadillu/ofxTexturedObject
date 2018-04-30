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

	static TexturedObjectStats& one(); //singleton

	void addTextureObject(TexturedObject * obj);
	void removeTextureObject(TexturedObject * obj);

	void update();
	void draw(int x, int y);

	std::string getStatsAsText();
	static std::string bytesToHumanReadable(uint64_t bytes, int decimalPrecision);

private:

	std::unordered_map<TexturedObject*, std::vector<TexturedObject::TexturedObjectSizeUnit>* > objectTexturesData;

	int loadedTexturesCount;
	int loadedTextureRealCount;
	int countBySize[TEXTURE_OBJECT_NUM_SIZES];

	uint64_t pixelsInGPU;

	ofMutex mutex;

};

