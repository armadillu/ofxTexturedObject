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

using namespace std;

#define SETUP_CHECK				if(!hasBeenSetup){ ofLogError() << "TexturedObject being used before setup!"; return; }
#define SETUP_CHECK_RET_NULL	if(!hasBeenSetup){ ofLogError() << "TexturedObject being used before setup!"; return NULL; }
#define SETUP_CHECK_RET_FALSE	if(!hasBeenSetup){ ofLogError() << "TexturedObject being used before setup!"; return false; }
#define TEX_EXISTS_CHECK		if(!textureExists(s, index)){ ofLogError() << "TexturedObject tex does not exist!"; return;}


TexturedObject::TexturedObject(){
	hasBeenSetup = false;
	resizeQuality = CV_INTER_CUBIC;
}


void TexturedObject::deleteWithGC(){

	SETUP_CHECK
	//see if any of our textures are being loaded
	//cancel loading if that's the case
	for(size_t j = 0; j < textures.size(); j++){
		map<TexturedObjectSize, TextureUnit>::iterator it = textures[j].sizes.begin();
		while(it != textures[j].sizes.end()){

			TextureUnit & texUnit = it->second; //out shortcut to the current texUnit
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

void TexturedObject::waitForThread(long ms){
	for (size_t j = 0; j < textures.size(); j++) {
		map<TexturedObjectSize, TextureUnit>::iterator it = textures[j].sizes.begin();
		while (it != textures[j].sizes.end()) {
			TextureUnit & texUnit = it->second; //out shortcut to the current texUnit
			if (texUnit.state == LOADING_TEXTURE) {
				if (texUnit.loader) {
					texUnit.loader->waitForThread(ms);
				}
			}
			++it;
		}
	}
}

#pragma mark -
// TEXTURE COMMANDS
ofTexture* TexturedObject::requestTexture(TexturedObjectSize s, int index, bool highPriority, bool withMipmaps){

	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		ofLogError("TexturedObject") << "TexturedObject tex does not exist!";
		return NULL;
	}

	TextureUnit & texUnit = textures[index].sizes[s];
	//ofLogNotice("TexturedObject")  << getInfo(texUnit.size, texUnit.index);

	bool addToQueue = resolveQueueRedundancies(texUnit, LOAD_TEXTURE);

	if(addToQueue){
		//ofLogNotice("TexturedObject") << "add command to queue";
		TextureCommand c;
		c.action = LOAD_TEXTURE;
		c.withMipmaps = withMipmaps;
		c.highPriority = highPriority;
		texUnit.pendingCommands.push_back(c);
	}
	return texUnit.texture;
}

void TexturedObject::releaseTexture(TexturedObjectSize s, int index, float delay){

	//ofLogNotice("TexturedObject");
	SETUP_CHECK
	if(!textureExists(s, index)){
		ofLogError("TexturedObject") << "TexturedObject tex does not exist!";
		return;
	}

	TextureUnit & texUnit = textures[index].sizes[s];

	if(delay <= 0.0f){

		//lets see if its cancels each other with the last pending command b4 we enque the command
		bool addToQueue = resolveQueueRedundancies(texUnit, UNLOAD_TEXTURE);
		if(addToQueue){
			TextureCommand c;
			c.action = UNLOAD_TEXTURE;
			texUnit.pendingCommands.push_back(c);
		}else{
			//ofLogNotice("TexturedObject")_NOTICE << "remove last command to queue - they cancel each other with this one" << getInfo(texUnit.size, texUnit.index);
		}
	}else{
		texUnit.scheduledUnloads.push_back(ofGetElapsedTimef() + delay);
	}
}
#pragma mark -

bool TexturedObject::resolveQueueRedundancies(TextureUnit& texUnit, TextureAction myAction){

	//lets see if my action cancels the previous one - b4 we enque the command
	bool addToQueue = false;
	if(texUnit.pendingCommands.size()){
		const TextureCommand & lastCommand = texUnit.pendingCommands.front();
		if(lastCommand.action == myAction){
			addToQueue = true;
		}else{ //2 actions cancel each other - whichever is the textLoad action, needs to get notified

			TextureEventArg event = TextureEventArg(true, this, texUnit.texture, texUnit.size, texUnit.index);
			event.absolutePath = getLocalTexturePath(texUnit.size, texUnit.index);

			if(texUnit.state == IDLE && isFullyLoaded(texUnit.size, texUnit.index)){ //if texObj is idle, it means the tex is already loaded
				pendingLoadEventNotifications.push_back(event);
				pendingReadyToDrawEventNotifications.push_back(event);
			}

			if(texUnit.state == LOADING_TEXTURE){ //once its 100% loaded (or ready to load) it will notify

				//it may be that user requests a tex load while tex is already loading
				//if we get really unlucky, it may be that it requests it after the event ReadyToDraw has triggered
				//but before the texture is fully loaded (so state == LOADING_TEXTURE). if that's the case, trigger it here manually.
				if(isReadyToDraw(texUnit.size, texUnit.index)){
					pendingReadyToDrawEventNotifications.push_back(event);
				}
			}
		}
	}else{
		addToQueue = true;
	}

	if(!addToQueue){ //remove the last item of the pending commands, as its cancels out with this one
		texUnit.pendingCommands.erase(texUnit.pendingCommands.begin() + texUnit.pendingCommands.size() - 1);
	}
	return addToQueue;
}


TexturedObject::~TexturedObject(){

	//ofLogNotice("TexturedObject") << "destructor TexturedObject we have " << textures.size() << " textures " << this;
	for(size_t i = 0; i < textures.size(); i++){
		map<TexturedObjectSize, TextureUnit>::iterator it = textures[i].sizes.begin();
		while(it != textures[i].sizes.end()){
			it->second.texture->clear();
			delete it->second.texture;
			++it;
		}
	}
	textures.clear();

	if(hasBeenSetup){
		//ofRemoveListener(ofEvents().update, this, &TexturedObject::update, OF_EVENT_ORDER_BEFORE_APP);
		TexturedObjectStats::one().removeTextureObject(this);
	}
}


void TexturedObject::setup(int numTextures, TexturedObjectSize s){
	vector<TexturedObjectSize> sizes;
	sizes.push_back(s);
	TexturedObject::setup(numTextures, sizes);
}


void TexturedObject::setup(int numTextures, vector<TexturedObjectSize> validImageSizes){

	if(hasBeenSetup){
        ofLogError("TexturedObject") << "TexturedObject already setup!"; return;
	}else{
		//ofAddListener(ofEvents().update, this, &TexturedObject::update, OF_EVENT_ORDER_BEFORE_APP);

		//alloc all ofTextures
		for(size_t j = 0; j < numTextures; j++){
			TexturedObjectSizeUnit allSizesUnit;
			for(size_t i = 0; i < validImageSizes.size(); i++){
				TexturedObjectSize s = (TexturedObjectSize)validImageSizes[i];
				TextureUnit unit;
				unit.texture = new ofTexture();
				unit.size = s;
				unit.index = j;

				allSizesUnit.sizes[s] = unit;
			}
			textures.push_back(allSizesUnit);
		}
		TexturedObjectStats::one().addTextureObject(this);
		hasBeenSetup = true;
	}
}


#pragma mark -

void TexturedObject::loadTexture(TextureCommand c, TextureUnit & texUnit){

	//ofLogNotice("TexturedObject") << "load tex " << getInfo(texUnit.size, texUnit.index);
	string path = getLocalTexturePath(texUnit.size, texUnit.index);

	if(texUnit.loadCount == 0){ //only load a texture when its not loaded already

		if(texUnit.loader == NULL){ //tex not being loaded
			storeTexPathInfo(texUnit, path); //store essential info about this texture to be able to retrieve it back once we get a "loaded" event out of the blue
			texUnit.loader = addToLoadQueue(texUnit.texture, c.withMipmaps, c.highPriority, path);
			texUnit.state = LOADING_TEXTURE;
			setFlags(texUnit, false, false, false, false);
			//ofLogNotice("TexturedObject") << "loading tex now! adding job to load queue " << getInfo(texUnit.size, texUnit.index);
		}else{
			ofLogError("TexturedObject") << "loader already exists!? tex already being requested to load!" << getInfo(texUnit.size, texUnit.index);
		}
	}else{

		if(texUnit.state == IDLE){ //if texObj is idle, it means the tex is already loaded
			TextureEventArg event = TextureEventArg(true, this, texUnit.texture, texUnit.size, texUnit.index);
			event.absolutePath = path;
			ofNotifyEvent(textureReadyToDraw, event, this);
			ofNotifyEvent(textureLoaded, event, this);
		}

		if(texUnit.state == LOADING_TEXTURE){ //once its 100% loaded (or ready to load) it will notify

			//it may be that user requests a tex load while tex is already loading
			//if we get really unlucky, it may be that it requests it after the event ReadyToDraw has triggered
			//but before the texture is fully loaded (so state == LOADING_TEXTURE). if that's the case, trigger it here manually.
			if(isReadyToDraw(texUnit.size, texUnit.index)){
				TextureEventArg event = TextureEventArg(true, this, texUnit.texture, texUnit.size, texUnit.index);
				event.absolutePath = path;
				ofNotifyEvent(textureReadyToDraw, event, this);
			}
		}
	}

	texUnit.loadCount ++;
	texUnit.totalLoadCount++;
}



void TexturedObject::unloadTexture(TextureCommand c, TextureUnit & texUnit){

	//ofLogNotice("TexturedObject") << "unloadTexture " << getInfo(texUnit.size, texUnit.index);

	if(texUnit.loadCount == 1){ //last reference to this texture

		if(texUnit.state == LOADING_TEXTURE){ //currently loading, lets cancel the loading.
			//ofLogNotice("TexturedObject") << "we are in the middle of loading a texture - cant unload right away " << getInfo(texUnit.size, texUnit.index);
			texUnit.loader->stopLoadingAsap();
			texUnit.loader = NULL;
			texUnit.state = WAITING_FOR_CANCEL_TO_FINISH;
			setFlags(texUnit, false, false, false, true);

		}else{ //we can unload the tex right now
			texUnit.texture->clear();
			texUnit.texture->texData.width = 0.0f;
			texUnit.texture->texData.height = 0.0f;
			texUnit.state = IDLE;
			setFlags(texUnit, false, false, false, false);
		}
	}
	texUnit.loadCount --;
	checkLoadCount(texUnit);
}


#pragma mark -

void TexturedObject::update(float timeNow){

	for(size_t j = 0; j < textures.size(); j++){

		map<TexturedObjectSize, TextureUnit>::iterator it = textures[j].sizes.begin();

		while(it != textures[j].sizes.end()){

			TextureUnit & texUnit = it->second; //out shortcut to the current texUnit

			//see if any of the future scheduled unloads is due
			for(size_t i = 0; i < texUnit.scheduledUnloads.size(); i++ ){
				if( texUnit.scheduledUnloads[i] < timeNow){
					texUnit.scheduledUnloads.erase(texUnit.scheduledUnloads.begin());
					bool addToQueue = resolveQueueRedundancies(texUnit, UNLOAD_TEXTURE);
					if(addToQueue){
						//ofLogNotice("TexturedObject") << "SCHEDULED UNLOAD - add UNLOAD command to queue";
						TextureCommand c;
						c.action = UNLOAD_TEXTURE;
						texUnit.pendingCommands.push_back(c);
					}
				}
			}

			switch (texUnit.state) {

				case IDLE:{

					while(texUnit.pendingCommands.size()){
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
						ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent r = texUnit.loaderResponses[0];
						texUnit.loaderResponses.erase(texUnit.loaderResponses.begin());

						if (r.readyToDraw){ //we got a ready to draw while waiting for cancel, lets ignore it.
							ofLogWarning("TexturedObject") << "we got a ready to draw while waiting for cancel, lets ignore it" << getInfo(texUnit.size, texUnit.index);
						}else{
							if (r.canceledLoad){ //tex load was cancelled, we can now clear the texture
								texUnit.texture->clear();
								texUnit.texture->texData.width = 0.0f;
								texUnit.texture->texData.height = 0.0f;
							}else{
								ofLogFatalError("TexturedObject") << "wtf! expected a cancel, got a load!" << getInfo(texUnit.size, texUnit.index);
							}
							texUnit.state = IDLE;
						}
					}
				}break;

				case LOADING_TEXTURE:{

					while(texUnit.loaderResponses.size()){
						ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent r = texUnit.loaderResponses[0];
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
									TextureEventArg event = TextureEventArg(true, this, texUnit.texture, texUnit.size, texUnit.index);
									event.absolutePath = r.texturePath;
									event.elapsedTime = r.elapsedTime;
									ofNotifyEvent(textureLoaded, event, this);
									texUnit.state = IDLE;
									setFlags(texUnit, true, true, false, false);
								}else{
									if(r.readyToDraw){ //tex is ready to draw but not fully loaded, notify end user
										TextureEventArg event = TextureEventArg(true, this, texUnit.texture, texUnit.size, texUnit.index);
										event.absolutePath = r.texturePath;
										event.elapsedTime = r.elapsedTime;
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
							ofLogError("TexturedObject") << "\ntex load did fail!" << getInfo(texUnit.size, texUnit.index);
							texUnit.loader = NULL;
							texUnit.state = IDLE;
							setFlags(texUnit, false, false, true, false);
							TextureEventArg event = TextureEventArg(false, this, texUnit.texture, texUnit.size, texUnit.index);
							event.absolutePath = r.texturePath;
							event.elapsedTime = r.elapsedTime;
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

	while(pendingReadyToDrawEventNotifications.size() > 0){
		ofNotifyEvent(textureReadyToDraw, pendingReadyToDrawEventNotifications.front(), this);
		pendingReadyToDrawEventNotifications.erase(pendingReadyToDrawEventNotifications.begin());
	}

	while(pendingLoadEventNotifications.size() > 0){
		ofNotifyEvent(textureLoaded, pendingLoadEventNotifications.front(), this);
		pendingLoadEventNotifications.erase(pendingLoadEventNotifications.begin());
	}

}


#pragma mark -


void TexturedObject::textureIsReadyToDraw(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent &e){

	TexturedObjectSize s = texToTexInfo[e.tex].size;
	int index = texToTexInfo[e.tex].index;

	TEX_EXISTS_CHECK

	TextureUnit & texUnit = textures[index].sizes[s];

	if(texUnit.state == LOADING_TEXTURE){
		texUnit.loaderResponses.push_back(e);
	}else{
		ofLogNotice("TexturedObject") << "ignoring ready to draw event on canceled texture load" << getInfo(s, index) << " event loadr:" << e.who;
	}
}


void TexturedObject::textureDidLoad(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent &e){

	TexturedObjectSize s = texToTexInfo[e.tex].size;
	int index = texToTexInfo[e.tex].index;
	//ofLogNotice("TexturedObject") << "textureDidLoad event " << getInfo(s, index) << " event loadr:" << e.who;

	TEX_EXISTS_CHECK

	TextureUnit & texUnit = textures[index].sizes[s];
	texUnit.loaderResponses.push_back(e);
}


#pragma mark -

void TexturedObject::setFlags(TextureUnit& texUnit, bool loaded, bool readytoDraw, bool error, bool canceled){
	texUnit.fullyloaded = loaded;
	texUnit.readyToDraw = readytoDraw;
	texUnit.errorLoading = error;
	texUnit.canceledLoad = canceled;
}

void TexturedObject::storeTexPathInfo(TextureUnit & texUnit, const string & path){
	TextureInfo texInfo;
	texInfo.index = texUnit.index;
	texInfo.size = texUnit.size;
	texToTexInfo[texUnit.texture] = texInfo;
}

void TexturedObject::checkLoadCount(TextureUnit & u){
	if(u.loadCount < 0){
		ofLogError("TexturedObject") << "Texture retain count is Negative!! resetting to zero. This can lead to texture leaks!" << getInfo(u.size, u.index);
		u.loadCount = 0;
	}
}


bool TexturedObject::textureExists(TexturedObjectSize s, int index){

	bool exists = true;
	if(index < 0 || index > textures.size() - 1){
		exists = false;
	}else{
		map<TexturedObjectSize, TextureUnit>::iterator it = textures[index].sizes.find(s);
		if(it == textures[index].sizes.end()){
			exists = false;
		}
	}
	return exists;
}


bool TexturedObject::isLoadingTextures(){

	SETUP_CHECK_RET_FALSE
	int numLoading = 0;
	for(size_t j = 0; j < textures.size(); j++){
		map<TexturedObjectSize, TextureUnit>::iterator it = textures[j].sizes.begin();
		while(it != textures[j].sizes.end()){
			TextureUnit & texUnit = it->second; //out shortcut to the current texUnit
			++it;
			if(texUnit.state != IDLE) numLoading ++;
		}
	}
	return (numLoading > 0);
}




int TexturedObject::getRetainCount(TexturedObjectSize s, int index){
	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		return 0;
	}
	return textures[index].sizes[s].loadCount;
}


int TexturedObject::getTotalLoadCount(TexturedObjectSize s, int index){
	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		return 0;
	}
	return textures[index].sizes[s].totalLoadCount;
}

bool TexturedObject::isReadyToDraw(TexturedObjectSize s, int index){
	SETUP_CHECK_RET_FALSE
	if(!textureExists(s, index)){
		return false;
	}
	return textures[index].sizes[s].readyToDraw;
}


bool TexturedObject::isLoading(TexturedObjectSize s, int index){
	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		return textures[index].sizes[s].state == LOADING_TEXTURE;
	}
	return false;
}

bool TexturedObject::isUnloading(TexturedObjectSize s, int index){
	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		return textures[index].sizes[s].state == WAITING_FOR_CANCEL_TO_FINISH;
	}
	return false;
}

bool TexturedObject::isWaitingForCancelToFinish(TexturedObjectSize s, int index){
	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		return false;
	}
	return textures[index].sizes[s].state == WAITING_FOR_CANCEL_TO_FINISH;
}


bool TexturedObject::isFullyLoaded(TexturedObjectSize s, int index){

	SETUP_CHECK_RET_FALSE
	if(!textureExists(s, index)){
		return false;
	}
	return textures[index].sizes[s].fullyloaded;
}


bool TexturedObject::gotErrorLoading(TexturedObjectSize s, int index){
	SETUP_CHECK_RET_FALSE
	return textures[index].sizes[s].errorLoading;
}

bool TexturedObject::loadWasCanceled(TexturedObjectSize s, int index){
	SETUP_CHECK_RET_FALSE
	return textures[index].sizes[s].canceledLoad;
}


ofTexture* TexturedObject::getLoadingTexture(){
	if(TexturedObjectConfig::one().loadingTex.size() == 0) return NULL;
	int index = (int)(ofGetFrameNum() * 0.3) % TexturedObjectConfig::one().loadingTex.size();
	return TexturedObjectConfig::one().loadingTex[index];
}


ofTexture* TexturedObject::getTexture(TexturedObjectSize s, int index){

	SETUP_CHECK_RET_NULL
	if(!textureExists(s, index)){
		ofLogError("TexturedObject") << "TexturedObject tex does not exist!";
		return NULL;
	}

	TextureUnit & texUnit = textures[index].sizes[s];

	if(texUnit.loadCount > 0){

		switch (texUnit.state) {
			case IDLE:
				if(texUnit.errorLoading) return TexturedObjectConfig::one().errorTex;
				else return texUnit.texture;

			case LOADING_TEXTURE:
				if(texUnit.readyToDraw) return texUnit.texture;
				return getLoadingTexture();
		}
	}else{
		if(texUnit.state == WAITING_FOR_CANCEL_TO_FINISH){
			return TexturedObjectConfig::one().cancelingTex;
		}
	}
	return TexturedObjectConfig::one().missingTex;
}


ofTexture* TexturedObject::getRealTexture(TexturedObjectSize s, int index){
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
														mipmap,	/*MIP-MAPS - create them!*/
														!mipmap, //only do ARB if no mipmaps are requested
														resizeQuality,
														highPriority /*load priority - put at end of queue or beginning*/
														);

	ofAddListener(loader->textureReady, this, &TexturedObject::textureDidLoad); //subscribe to img loaded event
	ofAddListener(loader->textureDrawable, this, &TexturedObject::textureIsReadyToDraw); //subscribe to img ready event
	return loader;
}


string TexturedObject::getInfo(TexturedObjectSize s, int index){
	stringstream info;
	string tab = "  ";
	info << "\n" + tab + "obj: "  << this
		<< tab + "loadC: " << textures[index].sizes[s].loadCount
		<< tab + "texIdx: " << index
		<< tab + "alloc: " << string(textures[index].sizes[s].texture->isAllocated() ? "yes": "no")
		<< tab + "tex: " << textures[index].sizes[s].texture
		//<< "readyToDraw: " << textures[index].sizes[s].readyToDraw
		<< tab + "waitingForCancelToFinish: " << string((textures[index].sizes[s].state == WAITING_FOR_CANCEL_TO_FINISH) ? " 1" : "0")
		<< tab + "canceled: " << textures[index].sizes[s].canceledLoad
		//<< "pendClear: " << texturesPendingClear.size()
		<< tab + "loader: " << textures[index].sizes[s].loader
		<< tab + "file: " << getLocalTexturePath(s, index)
		//<< " state: " << textures[index].sizes[s].state;
	;
 	return info.str();
}

std::string TexturedObject::toString(TexturedObjectSize s){

	switch (s) {

			/// ADD / MODIFY texture sizes AS NEEDED TO MATCH YOUR ENUM LIST!!
		case TEXTURE_SMALL: return "TEXTURE_SMALL";
		case TEXTURE_MEDIUM: return "TEXTURE_MEDIUM";
		case TEXTURE_LARGE: return "TEXTURE_LARGE";
		case TEXTURE_ORIGINAL: return "TEXTURE_ORIGINAL";
	}
	return "TEXTURE_OBJECT_UNKNOWN";
}

#undef SETUP_CHECK
#undef SETUP_CHECK_RET_NULL
#undef SETUP_CHECK_RET_FALSE
#undef TEX_EXISTS_CHECK
