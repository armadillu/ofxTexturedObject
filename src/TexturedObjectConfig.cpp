//
//  TexturedObjectConfig.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 16/03/15.
//
//

#include "TexturedObjectConfig.h"

TexturedObjectConfig::TexturedObjectConfig(){
	missingTex = NULL;
	errorTex = NULL;
	cancelingTex = NULL;
	unloadTextureDelayDefaults = 3.0;
}


void TexturedObjectConfig::setTextures(ofTexture* missing,
									   ofTexture* error,
									   ofTexture* canceling,
									   vector<ofTexture*> loading){
	missingTex = missing;
	errorTex = error;
	loadingTex = loading;
	cancelingTex = canceling;
}


void TexturedObjectConfig::setDefaultTextureUnloadDelay(float seconds){
	unloadTextureDelayDefaults = seconds;
}


float TexturedObjectConfig::getDefaultTextureUnloadDelay(){
	return unloadTextureDelayDefaults;
}
