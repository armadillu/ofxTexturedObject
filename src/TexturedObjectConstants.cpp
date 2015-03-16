//
//  TexturedObjectConstants.h
//
//
//  Created by Oriol Ferrer Mesià on 09/11/14.
//
//

#include "TexturedObjectSizes.h"



std::string toString(TexturedObjectSize s){

	switch (s) {

		/// ADD / MODIFY texture sizes AS NEEDED TO MATCH YOUR ENUM LIST!! 
		case TEXTURE_SMALL: return "TEXTURE_SMALL";
		case TEXTURE_MEDIUM: return "TEXTURE_MEDIUM";
		case TEXTURE_LARGE: return "TEXTURE_LARGE";
		case TEXTURE_ORIGINAL: return "TEXTURE_ORIGINAL";
	}
	return "TEXTURE_OBJECT_UNKNOWN";
}