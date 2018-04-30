//
//  TexturedObjectGC.cpp
//  BaseApp
//
//  Created by Oriol Ferrer Mesià on 13/05/14.
//
//

#include "TexturedObjectGC.h"
#include "TexturedObject.h"
#include "TexturedObjectStats.h"

TexturedObjectGC* TexturedObjectGC::singleton = NULL;

TexturedObjectGC* TexturedObjectGC::instance(){

	if (!singleton){   // Only allow one instance of class to be generated.
		singleton = new TexturedObjectGC();
		singleton->setup();
	}
	return singleton;
}

void TexturedObjectGC::setup(){
	ofAddListener(ofEvents().update, this, &TexturedObjectGC::update);
}


void TexturedObjectGC::addToGarbageCollectorQueue(TexturedObject * p){
	mutex.lock();
	pendingDeletion.push_back(p);
	mutex.unlock();
}

void TexturedObjectGC::update(ofEventArgs &a){

	mutex.lock();
	std::vector<int> indicesToDelete;
	for(size_t i = 0; i < pendingDeletion.size(); i++){
		if (!pendingDeletion[i]->isLoadingTextures()){
			//TexturedObjectStats::one().removeTextureObject(pendingDeletion[i]);
			delete pendingDeletion[i];
			pendingDeletion[i] = NULL;
			indicesToDelete.push_back(i);
		}else{
			ofLogNotice("TexturedObjectGC") << "still waiting to delete " << pendingDeletion[i] << "..." ;
		}
	}
	for(int i = indicesToDelete.size() - 1; i >= 0; i--){
		pendingDeletion.erase(pendingDeletion.begin() + indicesToDelete[i]);
	}
	mutex.unlock();
}
