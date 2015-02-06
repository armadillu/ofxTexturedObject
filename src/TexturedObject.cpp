//
//  TexturedObject.cpp
//  CollectionTable
//
//  Created by Oriol Ferrer Mesià on 06/11/14.
//
//

#include "TexturedObject.h"
#include "TexturedObjectStats.h"
#include "TexturedObjectGC.h"

ofTexture* TexturedObject::missingTex = NULL;
ofTexture* TexturedObject::errorTex = NULL;
ofTexture* TexturedObject::cancelingTex = NULL;
vector<ofTexture*> TexturedObject::loadingTex = vector<ofTexture*>();
float TexturedObject::unloadTextureDelayDefaults = 3.0;

TexturedObject::TexturedObject(){
	isSetup = false;
/*
	isAutoUpdating = false;
    #ifdef TARGET_OSX
    //sometimes crashes on windows
        TexturedObjectStats::one().addTextureObject(this);
    #endif
*/
}

void TexturedObject::deleteWithGC(){

	SETUP_CHECK
	//see if any of our textures are being loaded
	//cancel loading if that's the case
	for(int j = 0; j < textures.size(); j++){
		map<ImageSize, ImageUnit>::iterator it = textures[j].sizes.begin();
		while(it != textures[j].sizes.end()){

			ImageUnit & texUnit = it->second; //out shortcut to the current texUnit
			if(texUnit.state == LOADING_TEXTURE){
				texUnit.loader->stopLoadingAsap();
				texUnit.loader = NULL;
				texUnit.state = WAITING_FOR_CANCEL_TO_FINISH;
			}
			++it;
		}
	}
	//and then add to GC, so that we can delete when no texts are being loaded
	TexturedObjectGC::instance()->addToGarbageCollectorQueue(this);
}


// TEXTURE COMMANDS
ofTexture* TexturedObject::requestTexture(ImageSize s, int index, bool highPriority, bool withMipmaps){

	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		ofLogError("TexturedObject") << "TexturedObject tex does not exist!";
		return NULL;
	}

	ImageUnit & texUnit = textures[index].sizes[s];
	//LOG  << getInfo(texUnit.size, texUnit.index);

	bool addToQueue = resolveQueueRedundancies(texUnit, LOAD_TEXTURE);

	if(addToQueue){
		//LOG << "add command to queue";
		TextureCommand c;
		c.action = LOAD_TEXTURE;
		c.withMipmaps = withMipmaps;
		c.highPriority = highPriority;
		texUnit.pendingCommands.push_back(c);
	}else{
		//LOG << "remove last command to queue - they cancel each other with this one" << getInfo(texUnit.size, texUnit.index);
	}
	return texUnit.texture;
}

void TexturedObject::releaseTexture(ImageSize s, int index, float delay){

	//LOG;
	SETUP_CHECK
	if(!textureExists(s, index)){
		ofLogError() << "TexturedObject tex does not exist!";
		return;
	}

	ImageUnit & texUnit = textures[index].sizes[s];

	if(delay <= 0.0f){

		//lets see if its cancels each other with the last pending command b4 we enque the command
		bool addToQueue = resolveQueueRedundancies(texUnit, UNLOAD_TEXTURE);
		if(addToQueue){
			TextureCommand c;
			c.action = UNLOAD_TEXTURE;
			texUnit.pendingCommands.push_back(c);
		}else{
			//LOG_NOTICE << "remove last command to queue - they cancel each other with this one" << getInfo(texUnit.size, texUnit.index);
		}
	}else{
		texUnit.scheduledUnloads.push_back(ofGetElapsedTimef() + delay);
	}
}


bool TexturedObject::resolveQueueRedundancies(ImageUnit& texUnit, TextureAction myAction){

	//lets see if my action cancels the previous one - b4 we enque the command
	bool addToQueue = false;
	if(texUnit.pendingCommands.size()){
		TextureAction lastActionInPendingQueue = texUnit.pendingCommands[texUnit.pendingCommands.size()-1].action;
		if(lastActionInPendingQueue == myAction){
			addToQueue = true;
		}
	}else{
		addToQueue = true;
	}

	if(!addToQueue){ //remove the last item of the pending commands, as its cancels out with this one
		texUnit.pendingCommands.erase(texUnit.pendingCommands.begin() + texUnit.pendingCommands.size() - 1);
	}
	//resolveQueueRedundancies(texUnit);
	return addToQueue;
}

TexturedObject::~TexturedObject(){
	//todo delete textures structure!
	//LOG << "destructor TexturedObject we have " << textures.size() << " textures "<< this ;
	for(int i = 0; i < textures.size(); i++){
		map<ImageSize, ImageUnit>::iterator it = textures[i].sizes.begin();
		while(it != textures[i].sizes.end()){
			it->second.texture->clear();
			delete it->second.texture;
			++it;
		}
	}
	textures.clear();
}


void TexturedObject::setup(int numTextures, ImageSize s){
	vector<ImageSize> sizes;
	sizes.push_back(s);
	TexturedObject::setup(numTextures, sizes);
}


void TexturedObject::setup(int numTextures, vector<ImageSize> validImageSizes){

	if(isSetup){
        ofLogError() << "TexturedObject already setup!"; return;
    }

	//alloc all ofTextures
	for(int j = 0; j < numTextures; j++){
		ImageSizeUnit allSizesUnit;
		for(int i = 0; i < validImageSizes.size(); i++){
			ImageSize s = (ImageSize)validImageSizes[i];
			ImageUnit unit;
			unit.texture = new ofTexture();
			unit.size = s;
			unit.index = j;

			allSizesUnit.sizes[s] = unit;
		}
		textures.push_back(allSizesUnit);
	}
	isSetup = true;
}

void TexturedObject::registerToTextureStats(){	
	TexturedObjectStats::one().addTextureObject(this);
}


void TexturedObject::setTextures(ofTexture* missing, ofTexture* error, ofTexture* canceling, vector<ofTexture*> loading){
	missingTex = missing;
	errorTex = error;
	loadingTex = loading;
	cancelingTex = canceling;
}

#pragma mark -

void TexturedObject::loadTexture(TextureCommand c, ImageUnit & texUnit){

	//LOG << "load tex " << getInfo(texUnit.size, texUnit.index);

	if(texUnit.loadCount == 0){ //only load a texture when its not loaded already
/*
	TextureLoadEventArg arg;
	ofTexture * returnTex = NULL;

	if(index < textures.size() ){

		string path = getLocalImagePath(s, index);
        
        //cout<<"TexturedObject::loadTexture "<<path<<endl;
        
		ImageUnit & texUnit = textures[index].sizes[s];
		returnTex = textures[index].sizes[s].texture; //we return a ptr to the texture that will hold the requested image


		if(texUnit.loadCount == 0){

			//LOG << "starting tex load from disk";
			//store essential info about this texture to be able to retrieve it back once we get a "loaded" event out of the blue
			TextureInfo texInfo;
			texInfo.index = index;
			texInfo.size = s;
			texPathToTexInfo[path] = texInfo;

			if(texUnit.loader == NULL){ //tex not being loaded

				ProgressiveTextureLoadQueue * q = ProgressiveTextureLoadQueue::instance();
				ofxProgressiveTextureLoad * loader = q->loadTexture(path,
																	textures[index].sizes[s].texture,
																	true,
																	IMAGE_RESIZE_QUALITY,
																	highPriority
																	);
				texUnit.fullyloaded = false;
				texUnit.readyToDraw = false;
				texUnit.errorLoading = false;
				texUnit.canceledLoad = false;
				ofAddListener(loader->textureReady, this, &TexturedObject::textureDidLoad); //subscribe to img loaded event
				ofAddListener(loader->textureDrawable, this, &TexturedObject::textureIsReadyToDraw); //subscribe to img ready event
				texUnit.loader = loader;
			}else{
				LOG_ERROR << "loader already exists!? tex already being requested to load!" << getInfo(s, index);
				//TODO! store whoever requested the texture to notify them when the tex is ready!
			}

		}else{ //texture is loaded or being loaded
*/

		string path = getLocalTexturePath(texUnit.size, texUnit.index);

		if(texUnit.loader == NULL){ //tex not being loaded
			storeTexPathInfo(texUnit, path); //store essential info about this texture to be able to retrieve it back once we get a "loaded" event out of the blue
			texUnit.loader = addToLoadQueue(texUnit.texture, c.withMipmaps, c.highPriority, path);
			texUnit.state = LOADING_TEXTURE;
			setFlags(texUnit, false, false, false, false);
			//LOG << "loading tex now! adding job to load queue " << getInfo(texUnit.size, texUnit.index);

		}else{
			ofLogError("TexturedObject") << "loader already exists!? tex already being requested to load!" << getInfo(texUnit.size, texUnit.index);
		}
	}else{
		TextureLoadEventArg event = TextureLoadEventArg(true, this, texUnit.texture, texUnit.size, texUnit.index);
		ofNotifyEvent(textureLoaded, event, this);
	}

	texUnit.loadCount ++;
	texUnit.totalLoadCount++;
}



void TexturedObject::unloadTexture(TextureCommand c, ImageUnit & texUnit){

	//LOG << "unloadTexture " << getInfo(texUnit.size, texUnit.index);

	if(texUnit.loadCount == 1){ //last reference to this texture

		if(texUnit.state == LOADING_TEXTURE){ //currently loading, lets cancel the loading.
			//LOG << "we are in the middle of loading a texture - cant unload right away " << getInfo(texUnit.size, texUnit.index);
			texUnit.loader->stopLoadingAsap();
			texUnit.loader = NULL;
			texUnit.state = WAITING_FOR_CANCEL_TO_FINISH;
			setFlags(texUnit, false, false, false, true);

		}else{ //we can unload the tex right now
			texUnit.texture->clear();
			texUnit.texture->texData.width = 0.0f;
			texUnit.texture->texData.width = 0.0f;
			texUnit.state = IDLE;
			setFlags(texUnit, false, false, false, false);
		}
	}
	texUnit.loadCount --;
	checkLoadCount(texUnit);
}


#pragma mark -

void TexturedObject::update(){

	for(int j = 0; j < textures.size(); j++){
		map<ImageSize, ImageUnit>::iterator it = textures[j].sizes.begin();

		while(it != textures[j].sizes.end()){

			ImageUnit & texUnit = it->second; //out shortcut to the current texUnit

			//see if any of the future sceduled unloads is due
			for(int i = 0; i < texUnit.scheduledUnloads.size(); i++ ){
				if( texUnit.scheduledUnloads[i] < ofGetElapsedTimef()){
					texUnit.scheduledUnloads.erase(texUnit.scheduledUnloads.begin());
					bool addToQueue = resolveQueueRedundancies(texUnit, UNLOAD_TEXTURE);
					if(addToQueue){
						//LOG << "SCHEDULED UNLOAD - add UNLOAD command to queue";
						TextureCommand c;
						c.action = UNLOAD_TEXTURE;
						texUnit.pendingCommands.push_back(c);
					}else{
						//LOG << " SCHEDULED UNLOAD - remove last command to queue - they cancel each other with this one";
					}
				}
			}


			switch (texUnit.state) {

				case IDLE:{

					if(texUnit.pendingCommands.size()){
						TextureCommand c = texUnit.pendingCommands[0];
						texUnit.pendingCommands.erase(texUnit.pendingCommands.begin());

						switch (c.action) {

							case LOAD_TEXTURE:
								loadTexture(c, texUnit);
								break;

							case UNLOAD_TEXTURE:
								unloadTexture(c, texUnit);
								break;
						}
					}
				}break;

				case WAITING_FOR_CANCEL_TO_FINISH:{

					if(texUnit.loaderResponses.size()){
						ofxProgressiveTextureLoad::textureEvent r = texUnit.loaderResponses[0];
						texUnit.loaderResponses.erase(texUnit.loaderResponses.begin());

						if (r.readyToDraw){ //we got a ready to draw while waiting for cancel, lets ignore it.
							ofLogWarning("TexturedObject") << "we got a ready to draw while waiting for cancel, lets ignore it" << getInfo(texUnit.size, texUnit.index);
						}else{
							if (r.canceledLoad){ //tex load was cancelled, we can now clear the texture
								texUnit.texture->clear();
								texUnit.texture->texData.width = 0.0f;
								texUnit.texture->texData.width = 0.0f;
							}else{
								ofLogFatalError("TexturedObject") << "wtf! expected a cancel, got a load!" << getInfo(texUnit.size, texUnit.index);
							}
							texUnit.state = IDLE;
						}
					}
				}break;

				case LOADING_TEXTURE:{

					while(texUnit.loaderResponses.size()){
						ofxProgressiveTextureLoad::textureEvent r = texUnit.loaderResponses[0];
						texUnit.loaderResponses.erase(texUnit.loaderResponses.begin());

						if (r.ok){ //whatever u were doing, did it go ok?
							if (r.canceledLoad){
								ofLogFatalError("TexturedObject") << "wtf! expected a load, got a cancel!" << getInfo(texUnit.size, texUnit.index);
								texUnit.loader = NULL;
								texUnit.state = IDLE;
								setFlags(texUnit, false, false, true, true);
							}else{
								if(r.fullyLoaded){ //tex is ready, notify end user!
									texUnit.loader = NULL;
									TextureLoadEventArg event = TextureLoadEventArg(true, this, texUnit.texture, texUnit.size, texUnit.index);
									ofNotifyEvent(textureLoaded, event, this);
									texUnit.state = IDLE;
									setFlags(texUnit, true, true, false, false);
								}else{
									if(r.readyToDraw){ //tex is ready to draw but not fully loaded, notify end user
										TextureLoadEventArg event = TextureLoadEventArg(true, this, texUnit.texture, texUnit.size, texUnit.index);
										ofNotifyEvent(textureReadyToDraw, event, this);
										setFlags(texUnit, false, true, false, false);
									}else{ //tex is not ready to draw and not fully loaded - wtf?
										texUnit.loader = NULL;
										texUnit.state = IDLE;
										setFlags(texUnit, false, false, true, false);
										ofLogFatalError("TexturedObject") << "wtf!! loading tex but got event with nothing?" << getInfo(texUnit.size, texUnit.index);
									}
								}
							}
						}else{
							ofLogError("TexturedObject") << "tex load did fail!" << getInfo(texUnit.size, texUnit.index);
							texUnit.loader = NULL;
							texUnit.state = IDLE;
							setFlags(texUnit, false, false, true, false);
							TextureLoadEventArg event = TextureLoadEventArg(false, this, texUnit.texture, texUnit.size, texUnit.index);
							ofNotifyEvent(textureLoaded, event, this);
						}
					}
					if(texUnit.state == LOADING_TEXTURE){
						//have a sneak peak - if next command is unload - stop this load!
						if(texUnit.pendingCommands.size()){
							TextureCommand c = texUnit.pendingCommands[0];
							if (c.action == UNLOAD_TEXTURE){
								texUnit.pendingCommands.erase(texUnit.pendingCommands.begin());
								unloadTexture(c, texUnit);
							}
						}
					}
				}break;
			}
			++it;
		}
	}
}


#pragma mark -


void TexturedObject::textureIsReadyToDraw(ofxProgressiveTextureLoad::textureEvent &e){

	ImageSize s = texPathToTexInfo[e.texturePath].size;
	int index = texPathToTexInfo[e.texturePath].index;
	//LOG << "textureIsReadyToDraw event " << getInfo(s, index) << " event loadr:" << e.who;

	TEX_EXISTS_CHECK
	ImageUnit & texUnit = textures[index].sizes[s];

	if(texUnit.state == LOADING_TEXTURE){
		texUnit.loaderResponses.push_back(e);
	}else{
		ofLogNotice("TexturedObject") << "ignoring ready to draw event on canceled texture load" << getInfo(s, index) << " event loadr:" << e.who;
	}
}


void TexturedObject::textureDidLoad(ofxProgressiveTextureLoad::textureEvent &e){

	ImageSize s = texPathToTexInfo[e.texturePath].size;
	int index = texPathToTexInfo[e.texturePath].index;
	//LOG << "textureDidLoad event " << getInfo(s, index) << " event loadr:" << e.who;

	TEX_EXISTS_CHECK

	ImageUnit & texUnit = textures[index].sizes[s];
	texUnit.loaderResponses.push_back(e);
}


#pragma mark -

void TexturedObject::setFlags(ImageUnit& texUnit, bool loaded, bool readytoDraw, bool error, bool canceled){
	texUnit.fullyloaded = loaded;
	texUnit.readyToDraw = readytoDraw;
	texUnit.errorLoading = error;
	texUnit.canceledLoad = canceled;
}

void TexturedObject::storeTexPathInfo(ImageUnit & texUnit, const string & path){
	TextureInfo texInfo;
	texInfo.index = texUnit.index;
	texInfo.size = texUnit.size;
	texPathToTexInfo[path] = texInfo;
}

void TexturedObject::checkLoadCount(ImageUnit & u){
	if(u.loadCount < 0){
		ofLogError("TexturedObject") << "trying to load tex, but got negative count! " << getInfo(u.size, u.index);
		u.loadCount = 0;
	}
}


bool TexturedObject::textureExists(ImageSize s, int index){

	bool exists = true;
	if(index < 0 || index > textures.size() - 1){
		exists = false;
	}else{
		map<ImageSize, ImageUnit>::iterator it = textures[index].sizes.find(s);
		if(it == textures[index].sizes.end()){
			exists = false;
		}
	}
	return exists;
}


bool TexturedObject::isLoadingTextures(){

	SETUP_CHECK_RET_FALSE
	int numLoading = 0;
	for(int j = 0; j < textures.size(); j++){
		map<ImageSize, ImageUnit>::iterator it = textures[j].sizes.begin();
		while(it != textures[j].sizes.end()){
			ImageUnit & texUnit = it->second; //out shortcut to the current texUnit
			++it;
			if(texUnit.state != IDLE) numLoading ++;
		}
	}
	return (numLoading > 0);
}




int TexturedObject::getRetainCount(ImageSize s, int index){
	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		return 0;
	}
	return textures[index].sizes[s].loadCount;
}


int TexturedObject::getTotalLoadCount(ImageSize s, int index){
	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		return 0;
	}
	return textures[index].sizes[s].totalLoadCount;
}

bool TexturedObject::isReadyToDraw(ImageSize s, int index){
	SETUP_CHECK_RET_FALSE
	if(!textureExists(s, index)){
		return false;
	}
	return textures[index].sizes[s].readyToDraw;
}


bool TexturedObject::isWaitingForCancelToFinish(ImageSize s, int index){
	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		return 0;
	}
	return textures[index].sizes[s].state == WAITING_FOR_CANCEL_TO_FINISH;
}


bool TexturedObject::isFullyLoaded(ImageSize s, int index){

	SETUP_CHECK_RET_FALSE
	if(!textureExists(s, index)){
		return false;
	}
	return textures[index].sizes[s].fullyloaded;
}


bool TexturedObject::gotErrorLoading(ImageSize s, int index){
	SETUP_CHECK_RET_FALSE
	return textures[index].sizes[s].errorLoading;
}

bool TexturedObject::loadWasCanceled(ImageSize s, int index){
	SETUP_CHECK_RET_FALSE
	return textures[index].sizes[s].canceledLoad;
}


ofTexture* TexturedObject::getLoadingTexture(){
	if(loadingTex.size() == 0) return NULL;
	int index = (int)(ofGetFrameNum() * 0.3) % loadingTex.size();
	return loadingTex[index];
}


ofTexture* TexturedObject::getTexture(ImageSize s, int index){

	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		ofLogError("TexturedObject") << "TexturedObject tex does not exist!";
		return NULL;
	}

	ImageUnit & texUnit = textures[index].sizes[s];

	if(texUnit.loadCount > 0){

		switch (texUnit.state) {
			case IDLE:
				if(texUnit.errorLoading) return errorTex;
				else return texUnit.texture;

			case LOADING_TEXTURE:
				if(texUnit.readyToDraw) return texUnit.texture;
				return getLoadingTexture();

		}
	}else{
		if(texUnit.state == WAITING_FOR_CANCEL_TO_FINISH){
			return cancelingTex;
		}
	}
	return missingTex;
}

ofTexture* TexturedObject::getRealTexture(ImageSize s, int index){
	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		ofLogError("TexturedObject") << "TexturedObject tex does not exist!";
		return NULL;
	}
	return textures[index].sizes[s].texture;
}


ofxProgressiveTextureLoad* TexturedObject::addToLoadQueue(ofTexture * tex, bool mipmap, bool highPriority, string path){

	ProgressiveTextureLoadQueue * q = ProgressiveTextureLoadQueue::instance();
	ofxProgressiveTextureLoad * loader = q->loadTexture(path,	/*file path*/
														tex,	/*ofTexture* where to load*/
														true,	/*MIP-MAPS - create them!*/
														IMAGE_RESIZE_QUALITY,
														highPriority /*load priority - put at end of queue or beginning*/
														);
	ofAddListener(loader->textureReady, this, &TexturedObject::textureDidLoad); //subscribe to img loaded event
	ofAddListener(loader->textureDrawable, this, &TexturedObject::textureIsReadyToDraw); //subscribe to img ready event
	return loader;
}


string TexturedObject::getInfo(ImageSize s, int index){
	stringstream info;
	info << " obj: "  << this
		<< " loadC: " << textures[index].sizes[s].loadCount
		<< " texIdx: " << index
		<< " alloc: " << string(textures[index].sizes[s].texture->isAllocated() ? "yes": "no") 
		<< " tex: " << textures[index].sizes[s].texture
		//<< " readyToDraw: " << textures[index].sizes[s].readyToDraw
		<< " waitingForCancelToFinish: " << string((textures[index].sizes[s].state == WAITING_FOR_CANCEL_TO_FINISH) ? " 1" : "0")
		<< " canceled: " << textures[index].sizes[s].canceledLoad
		//<< " pendClear: " << texturesPendingClear.size()
		<< " loader: " << textures[index].sizes[s].loader
		<< " state: " << textures[index].sizes[s].state;
 	return info.str();
}

