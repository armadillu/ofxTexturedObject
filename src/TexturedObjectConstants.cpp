//
//  TexturedObjectConstants.cpp
//  CollectionTable
//
//  Created by Oriol Ferrer Mesià on 09/11/14.
//
//

#include "TexturedObjectConstants.h"


ImageSize getImageSizeForString(string s){
	if (s == "n") return IMG_256;
	if (s == "z") return IMG_600;
	if (s == "b") return IMG_1024;
	if (s == "k") return IMG_2048;
	if (s == "lp") return IMG_3000;
	if (s == "o") return IMG_ORIGINAL;
	return IMG_UNKNOWN;
}


string getStringForImageSize(ImageSize s){
	if (s == IMG_256) return "n"; //256px
	if (s == IMG_600) return "z"; //600px
	if (s == IMG_1024) return "b";	//1024px
	if (s == IMG_2048) return "k";	//2048px
	if (s == IMG_3000) return "lp";
	if (s == IMG_ORIGINAL) return "o"; //3000px, variable!
	return "?";
}

string getHumanStringForImageSize(ImageSize s){
	if (s == IMG_256) return "256px"; //256px
	if (s == IMG_600) return "600px"; //600px
	if (s == IMG_1024) return "1024px";	//1024px
	if (s == IMG_2048) return "2048px";//2048px
	if (s == IMG_3000) return "3000px";//2048px
	if (s == IMG_ORIGINAL) return "original";
	return "?";
}


vector<ImageSize> getValidImageSizes(){
	vector<ImageSize> sizes;
	for(int i = 0; i < IMG_NUM_SIZES; i++){
		ImageSize s = ImageSize(i);
		if(imageOkToDownload(s)){
			sizes.push_back(s);
		}
	}
	return sizes;
}

//this sets what image sizes we download!
bool imageOkToDownload(ImageSize s){
	bool r = (
			  s == RIVER_IMAGE_SIZE
			  ||
			  s == ZOOMABLE_IMAGE_SIZE
			  );
	return r;
}
