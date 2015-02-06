//
//  TexturedObject.h
//  CollectionTable
//
//  Created by Oriol Ferrer Mesià on 06/11/14.
//
//

#ifndef __CollectionTable__TexturedObject__
#define __CollectionTable__TexturedObject__

#include "ofMain.h"
#include "TexturedObjectConstants.h"
#include "ofxProgressiveTextureLoad.h"
#include "ProgressiveTextureLoadQueue.h"


#if __cplusplus>=201103L || defined(_MSC_VER)
	#include <unordered_map>
	#include <memory>
#else
	#include <tr1/unordered_map>
	using std::tr1::unordered_map;
#endif


#define SETUP_CHECK				if(!isSetup){ ofLogError() << "TexturedObject being used before setup!"; return; }
#define SETUP_CHECK_RET_NULL	if(!isSetup){ ofLogError() << "TexturedObject being used before setup!"; return NULL; }
#define SETUP_CHECK_RET_FALSE	if(!isSetup){ ofLogError() << "TexturedObject being used before setup!"; return false; }
#define TEX_EXISTS_CHECK	if(!textureExists(s, index)){ ofLogError() << "TexturedObject tex does not exist!"; return;}

class TexturedObject{

	friend class TexturedObjectStats;
	friend class TexturedObjectGC;


public:

	TexturedObject();

	void update();

	struct TextureLoadEventArg{
		bool loadedOk;
		TexturedObject * obj;
		ofTexture* tex;
		ImageSize size;
		int textureIndex;
		TextureLoadEventArg(){
			tex = NULL;
			obj = NULL;
		};
		TextureLoadEventArg(bool ok, TexturedObject* o, ofTexture * tex_, ImageSize s, int index = 0){
			loadedOk = ok;
			obj = o;
			tex = tex_;
			size = s;
			textureIndex = index;
		}
	};


	// SHARED ACROSS ALL INSTANCES - CONFIG //////////////////////

	//supply texture* for differernt error states (only once - static)
	//loading can be animatd set of textures, they will be played back with loop over time
	static void setTextures(ofTexture* missing, ofTexture* error, ofTexture* canceling, vector<ofTexture*> loading);

	//global (across all isntances) default of unload delat (as a gehtto cache system)
	static void setDefaultTextureUnloadDelay(float seconds){ unloadTextureDelayDefaults = seconds;}
	static float getDefaultTextureUnloadDelay(){ return unloadTextureDelayDefaults; }


	// SETUP /////////////////////////////////////////////////////

	void setup(int numTextures, ImageSize);
	void setup(int numTextures, vector<ImageSize> validImageSizes );
	//MUST CALL when you setup your TexturedObject subclass object!
	//by calling TextureObject::setup(int, ImageSize);

	// register this object for texture stats
	void registerToTextureStats();
	//TODO implement de-register! or crash on delete!


	// TEXTURE COMMANDS //////////////////////////////////////////

	ofTexture* requestTexture(ImageSize s, int index, bool highPriority = false, bool withMipmaps = true);
	void releaseTexture(ImageSize s, int index, float delay = 0.0);

	// STATUS COMMANDS /////////////////////////////////////////////

	bool isReadyToDraw(ImageSize s, int index);
	bool isFullyLoaded(ImageSize s, int index);
	bool isWaitingForCancelToFinish(ImageSize s, int index);

	// TEXTURE GETTERS /////////////////////////////////////////////

	ofTexture* getTexture(ImageSize s, int index);
	//this is sligthly clever, it will give you the real tex* if the tex is loaded
	//or its ready to draw; but it will give you a "loading" tex* replacement if its
	//still loading, or a "error" tex* if there was a loading error, or "canceled" tex if tex load was canceled

	ofTexture* getRealTexture(ImageSize s, int index);
	//dumbed down version of the above, will give you
	//the same tex* (if the index & size are valid)
	//regardles of the tex being loaded or not


	int getNumTextures(){
        return textures.size();
    }

	bool isLoadingTextures(); //if any of the texture of the object being "worked on"?

	int getRetainCount(ImageSize s, int index); //how many loads are on that textures
	int getTotalLoadCount(ImageSize s, int index); //how many times has been loaded since created
	bool gotErrorLoading(ImageSize s, int index);
	bool loadWasCanceled(ImageSize s, int index);


	// SUBCLASSES MUST IMPLEMENT these methods ////////////////////////////

	//this should give you texture dimensions even if texture is not loaded
	virtual ofVec2f getTextureDimensions(ImageSize s, int index) = 0;
	//which means it might not be the actual texture size...
	//but they should be the same in most cases

	//supply a texture size and index, return the file path
	virtual string getLocalTexturePath(ImageSize, int index) = 0;


	// OBJECT DELETION /////////////////////////////////////////////////////

	// NEVER delete a TexturedObject directly!
	virtual void deleteWithGC() = 0; // call this instead of delete!
	//from that point on, you are not supposed to access the pointer or you might get a crash
	//the object will be deleted when possible by the background garbage collector.
	//you must implement this in your subclass if your subclass has stuff to dealocate
	//and from there, call TexturedObject::deleteWithGC()


	// public OF events here! ////////////////////////////////////////////////

	ofEvent<TextureLoadEventArg> textureLoaded; //completely loaded
	ofEvent<TextureLoadEventArg> textureReadyToDraw; //not fully loaded, but drawable! tex->getWidht/height report correct values

protected:

	virtual ~TexturedObject(); //dont use this! use deleteWithGC();


private:

	enum TextureAction{
		LOAD_TEXTURE,
		UNLOAD_TEXTURE,
		UNKNOWN_COMMAND
	};

	enum TextureState{
		LOADING_TEXTURE,
		WAITING_FOR_CANCEL_TO_FINISH,
		IDLE,
	};

	struct TextureCommand{
		TextureAction action;
		bool withMipmaps;
		bool highPriority;
	};

	struct ImageUnit{
		ImageSize											size;
		int													index;
		vector<TextureCommand>								pendingCommands;
		vector<ofxProgressiveTextureLoad::textureEvent>		loaderResponses;
		vector<float>										scheduledUnloads;
		TextureState										state;
		ofxProgressiveTextureLoad*							loader;
		ofTexture *											texture;
		int													loadCount;		//textures held by external objects
		int													totalLoadCount; //total tex loaded over time

		//might not need this below
		bool								fullyloaded;
		bool								errorLoading;
		bool								canceledLoad;
		bool								readyToDraw; // drawable, but not with all the resolution yet. will progressively get sharper
		ImageUnit(){
			state = IDLE;
			loadCount = totalLoadCount = 0;
			loader = NULL;
			texture = NULL;
			readyToDraw = fullyloaded = errorLoading = canceledLoad = false;
		}
	};

	struct ImageSizeUnit{
		map<ImageSize, ImageUnit> sizes;
	};

	struct TextureInfo{
		int index;
		ImageSize size;
	};

	bool textureExists(ImageSize s, int index);
	string getInfo(ImageSize s, int index);

	vector<ImageSizeUnit> textures;
	//vector of structs that holds all images of that object,
	//each image has several sizes available.

	//given a texture file path, store its ID and imgSize
	unordered_map<string, TextureInfo> texPathToTexInfo;

	//texture loader callbacks
	void textureDidLoad(ofxProgressiveTextureLoad::textureEvent &e);
	void textureIsReadyToDraw(ofxProgressiveTextureLoad::textureEvent &e);


	//return true if you should add your action to queue, false if not.
	bool resolveQueueRedundancies(ImageUnit& texUnit, TextureAction myAction);

	void setFlags(ImageUnit& texUnit, bool loaded, bool readytoDraw, bool error, bool canceled);

	struct DelayedTexUnloadInfo{
		ImageSize size;
		int textureIndex;
		float time;
	};

	bool isSetup;

	ofxProgressiveTextureLoad* addToLoadQueue( ofTexture* tex, bool mipmap, bool highPriority, string path);

	//static textures to be provided by user - global config across all instances
	static ofTexture* missingTex;
	static ofTexture* errorTex;
	static ofTexture* cancelingTex;
	static vector<ofTexture*> loadingTex;
	static float unloadTextureDelayDefaults;


	ofTexture* getLoadingTexture();

	void storeTexPathInfo(ImageUnit & u, const string & path);
	void checkLoadCount(ImageUnit & u);

	// ############################################################

	void loadTexture(TextureCommand c, ImageUnit & texUnit);
	//returns tex* where your tex will be loaded into,
	//so you can tell when you get a notification
	//(which also gives you the same pointer)
	//if that notification refers to the texture you expect
	//or if it's for someone else (in which case you should ignore)

	void unloadTexture(TextureCommand c, ImageUnit & texUnit);


};

#endif /* defined(__CollectionTable__TexturedObject__) */

