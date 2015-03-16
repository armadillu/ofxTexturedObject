//
//  TexturedObjectConstants.h
//
//
//  Created by Oriol Ferrer Mesià on 09/11/14.
//
//

#pragma once
#include <string>
#include "ofMain.h"

enum TexturedObjectSize{

	/// ADD/MODIFY TO SPECIFY YOUR IMAGE SIZES BELOW;
	/// MUST START FROM 0
	TEXTURE_SMALL = 0,
	TEXTURE_MEDIUM,
	TEXTURE_LARGE,
	TEXTURE_ORIGINAL,

	//DO NOT EDITE BELOW THIS!
	TEXTURE_OBJECT_NUM_SIZES,
	TEXTURE_OBJECT_UNKNOWN = -1
};

std::string toString(TexturedObjectSize s);
