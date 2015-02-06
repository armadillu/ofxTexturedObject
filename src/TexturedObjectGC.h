//
//  TexturedObjectGC.h
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 13/05/14.
//
//

#ifndef __BaseApp__TexturedObjectGC
#define __BaseApp__TexturedObjectGC

#include "ofMain.h"

class TexturedObject;

class TexturedObjectGC{

public:

	static TexturedObjectGC* instance();
	void addToGarbageCollectorQueue(TexturedObject* objToDelete);

private:

	void setup();
	TexturedObjectGC(){}; //use instance()!
	void update(ofEventArgs &a);


	vector<TexturedObject*> 	pendingDeletion;
	static TexturedObjectGC*	singleton;
	ofMutex 					mutex;
};

#endif
