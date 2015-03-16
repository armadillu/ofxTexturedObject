//
//  TexturedObjectConfig.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 16/03/15.
//
//

#ifndef __BaseApp__TexturedObjectConfig__
#define __BaseApp__TexturedObjectConfig__

#include "ofMain.h"

class TexturedObjectConfig{

	friend class TexturedObject;

public:

	TexturedObjectConfig();

	static TexturedObjectConfig& one(){
		static TexturedObjectConfig instance;
		return instance;
	}

	//supply texture* for different error states to show when you try draw textures that are not ready
	//loading can be animated set of textures, they will be played back with loop over time
	void setTextures(ofTexture* missing,
					 ofTexture* error,
					 ofTexture* canceling,
					 vector<ofTexture*> loading
					 );


	//by default, when a texture is asked to unload, how long to delay that operation (ghetto cache)
	void setDefaultTextureUnloadDelay(float seconds);
	float getDefaultTextureUnloadDelay();

private:

	ofTexture* missingTex;
	ofTexture* errorTex;
	ofTexture* cancelingTex;
	vector<ofTexture*> loadingTex;
	float unloadTextureDelayDefaults;

};

#endif /* defined(__BaseApp__TexturedObjectConfig__) */
