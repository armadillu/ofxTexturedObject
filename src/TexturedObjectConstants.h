//
//  TexturedObjectConstants.h
//  CollectionTable
//
//  Created by Oriol Ferrer Mesià on 09/11/14.
//
//

#ifndef __CollectionTable__TexturedObjectConstants__
#define __CollectionTable__TexturedObjectConstants__

#include "ofMain.h"


///////////////////////////////////////////////////////////////// the two image sizes used on the table

#define RIVER_IMAGE_SIZE							IMG_1024
#define ZOOMABLE_IMAGE_SIZE							IMG_2048

/////////////////////////////////////////////////////////////////

/*CV_INTER_NN, CV_INTER_LINEAR, CV_INTER_CUBIC, CV_INTER_AREA, CV_INTER_LANCZOS4 */
#define IMAGE_RESIZE_QUALITY						CV_INTER_CUBIC


enum ImageSize{
	RIVER_IMAGE_SIZE = 0,
	ZOOMABLE_IMAGE_SIZE,
	IMG_NUM_SIZES, 	//anything > 2 not to be used
	IMG_256,
	IMG_600,
	IMG_3000,
	IMG_ORIGINAL,
	IMG_UNKNOWN = -1 	//used at undefined state
};

// ENUM HELPER METHODS ////////////////////////////////////////////

ImageSize getImageSizeForString(string s);
string getStringForImageSize(ImageSize s);
string getHumanStringForImageSize(ImageSize s);

// defining whhich textures are used
vector<ImageSize> getValidImageSizes();
bool imageOkToDownload(ImageSize s);


#endif /* defined(__CollectionTable__TexturedObjectConstants__) */
