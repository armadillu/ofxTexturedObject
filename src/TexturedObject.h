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
#include "TexturedObjectSizes.h"
#include "TexturedObjectConfig.h"
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

	//TextureEvent definition
	struct TextureEventArg{
		bool 				loadedOk;
		TexturedObject * 	obj;
		ofTexture *			tex;
		TexturedObjectSize 	size;
		int 				textureIndex;
		string 				absolutePath;
		float 				elapsedTime;
		TextureEventArg(){
			tex = NULL;
			obj = NULL;
			elapsedTime = 0.0f;
		};
		TextureEventArg(bool ok, TexturedObject* o, ofTexture * tex_, TexturedObjectSize s, int index = 0){
			loadedOk = ok;
			obj = o;
			tex = tex_;
			size = s;
			textureIndex = index;
			elapsedTime = 0.0f;
		}
	};


	// SETUP ///////////////////////////////////////////////////////////////////////////////////

	/**
	 YOU MUST CALL THIS from your TexturedObject subclass object when you setup() it;
	 do so by calling TextureObject::setup(int, TexturedObjectSize);

		numTextures >> how many different textures does this object hold
		size/s >> for each texture, how many texture sizes are available (ie LOD)

	 **/
	
	void setup(int numTextures, TexturedObjectSize size);
	void setup(int numTextures, vector<TexturedObjectSize> sizes );


	// SUBCLASSES MUST IMPLEMENT these methods /////////////////////////////////////////////////

	//this should give you texture dimensions even if texture is not loaded.
	//It's your subclass's job to figure out those.
	virtual ofVec2f getTextureDimensions(TexturedObjectSize s, int index) = 0;

	//This is where your subclass specifies where actual texture files are.
	virtual string getLocalTexturePath(TexturedObjectSize, int index) = 0;


	// CONFIG //////////////////////////////////////////////////////////////////////////////////

	/**
	 Used when resizing original image to create mipmaps, can be:
	 CV_INTER_NN, CV_INTER_LINEAR, CV_INTER_CUBIC, CV_INTER_AREA
	 defaults to CV_INTER_CUBIC
	 **/

	void setResizeQuality(int resizeQ){ resizeQuality = resizeQ; }


	// TEXTURE COMMANDS ////////////////////////////////////////////////////////////////////////

	//Load a texture for this object
	//	texSize - what tex size to load
	// 	index - what texture index to load for that object.
	//	highPriority - will put the command at the beggining of the load queue
	//	withMipmaps - load mipmaps for this texture as you load id

	ofTexture* requestTexture(TexturedObjectSize texSize,
							  int index,
							  bool highPriority = false,
							  bool withMipmaps = true
							  );

	//call this when you dont need that texture anymore, and it will be unloaded
	// 	delay - works as a simple "cache", will unload after N seconds

	void releaseTexture(TexturedObjectSize s,
						int index,
						float delaySeconds = 0.0);


	// STATUS COMMANDS ///////////////////////////////////////////////////////////////////////////


	bool isLoading(TexturedObjectSize s, int index);
	bool isUnloading(TexturedObjectSize s, int index);

	bool isReadyToDraw(TexturedObjectSize s, int index);
	bool isFullyLoaded(TexturedObjectSize s, int index);
	bool isWaitingForCancelToFinish(TexturedObjectSize s, int index);

	bool isLoadingTextures(); //if ANY of the textures indexes / sizes of the object being "worked on"?


	// TEXTURE GETTERS ///////////////////////////////////////////////////////////////////////////

	//This is sligthly clever, it will give you the real tex* if the tex is loaded
	//or its ready to draw; but it will give you a "loading" tex* replacement if its
	//still loading, or a "error" tex* if there was a loading error,
	//or "canceled" tex if tex load was canceled

	ofTexture* getTexture(TexturedObjectSize s, int index);

	//Dumbed down version of the above, will give you
	//the same tex* (if the index & size are valid)
	//regardles of the tex being loaded or not
	//which means the returned texture might not be loaded, or mid loading,

	ofTexture* getRealTexture(TexturedObjectSize s, int index);

 	//How many textures does this object have (as specified on setup())

	int getNumTextures(){
        return textures.size();
    }

	int getRetainCount(TexturedObjectSize s, int index); //how many loads are on that textures
	int getTotalLoadCount(TexturedObjectSize s, int index); //how many times has been loaded since created
	bool gotErrorLoading(TexturedObjectSize s, int index);
	bool loadWasCanceled(TexturedObjectSize s, int index);


	// OBJECT DELETION ///////////////////////////////////////////////////////////////////////////

	//NEVER EVER EVER delete a TexturedObject directly, or you will get a crash if a thread is loading it!
	virtual void deleteWithGC() = 0; // call this instead of delete!

	//from that point on, you are not supposed to access the pointer or you might get a crash
	//the object will be deleted when possible by the background garbage collector (TexturedObjectGC).
	//you must implement this in your subclass if your subclass has stuff to dealocate
	//and from there, call TexturedObject::deleteWithGC()


	// PUBLIC OF EVENTS //////////////////////////////////////////////////////////////////////////

	ofEvent<TextureEventArg> textureLoaded; //completely loaded
	ofEvent<TextureEventArg> textureReadyToDraw; //not fully loaded, but drawable! tex->getWidht/height report correct values

protected:

	virtual ~TexturedObject(); //dont use this! use deleteWithGC();

private:

	void update(ofEventArgs &arg);

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

	struct TextureUnit{
		TexturedObjectSize											size;
		int													index;
		vector<TextureCommand>								pendingCommands;
		vector<ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent>		loaderResponses;
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
		TextureUnit(){
			state = IDLE;
			loadCount = totalLoadCount = 0;
			loader = NULL;
			texture = NULL;
			readyToDraw = fullyloaded = errorLoading = canceledLoad = false;
		}
	};

	struct TexturedObjectSizeUnit{
		map<TexturedObjectSize, TextureUnit> sizes;
	};

	struct TextureInfo{
		int index;
		TexturedObjectSize size;
	};

	bool textureExists(TexturedObjectSize s, int index);
	string getInfo(TexturedObjectSize s, int index);

	vector<TexturedObjectSizeUnit> textures;
	//vector of structs that holds all images of that object,
	//each image has several sizes available.

	//given a texture file path, store its ID and imgSize
	unordered_map<string, TextureInfo> texPathToTexInfo;

	//texture loader callbacks
	void textureDidLoad(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent &e);
	void textureIsReadyToDraw(ofxProgressiveTextureLoad::ProgressiveTextureLoadEvent &e);


	//return true if you should add your action to queue, false if not.
	bool resolveQueueRedundancies(TextureUnit& texUnit, TextureAction myAction);

	void setFlags(TextureUnit& texUnit, bool loaded, bool readytoDraw, bool error, bool canceled);

	struct DelayedTexUnloadInfo{
		TexturedObjectSize size;
		int textureIndex;
		float time;
	};

	bool isSetup;

	ofxProgressiveTextureLoad* addToLoadQueue( ofTexture* tex, bool mipmap, bool highPriority, string path);

	ofTexture* getLoadingTexture();

	void storeTexPathInfo(TextureUnit & u, const string & path);
	void checkLoadCount(TextureUnit & u);

	// ############################################################

	void loadTexture(TextureCommand c, TextureUnit & texUnit);
	//returns tex* where your tex will be loaded into,
	//so you can tell when you get a notification
	//(which also gives you the same pointer)
	//if that notification refers to the texture you expect
	//or if it's for someone else (in which case you should ignore)

	void unloadTexture(TextureCommand c, TextureUnit & texUnit);

	//those are convering the notifications that would have been lost otherwise
	//bc they are "eaten up" by queue redundancies (ie "load + unload + load" = "load")
	//they will trigger both textureReadyToDraw and textureLoaded events
	vector<TextureEventArg> pendingLoadEventNotifications;

	int resizeQuality;

};

#endif /* defined(__CollectionTable__TexturedObject__) */

