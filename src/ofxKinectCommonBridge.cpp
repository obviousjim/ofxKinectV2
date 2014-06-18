#include "ofxKinectCommonBridge.h"

//================================================================================================================
// common bridge
//================================================================================================================


ofxKinectCommonBridge::ofxKinectCommonBridge(){
	hKinect = NULL;

	pDepthFrame = NULL;
	pColorFrame = NULL;

	beginMappingColorToDepth = false;
	bNeedsUpdateSkeleton = false;
	bUsingBodyIndex = false;
	bNeedsUpdateBodyIndex = false;
	bIsFrameNewVideo = false;
	bNeedsUpdateVideo = false;
	bIsFrameNewDepth = false;
	bNeedsUpdateDepth = false;
	bVideoIsInfrared = false;
	bVideoIsColor = false;
	bInited = false;
	bStarted = false;

	mappingColorToDepth = false;
	mappingDepthToColor = false;

	bUsingSkeletons = false;
	bUsingDepth = false;
  	bUseTexture = true;
	bUseFloatTexture = false;
	bProgrammableRenderer = false;
	
	setDepthClipping();


	skeletonDrawOrder[0] = make_pair<JointType, JointType>(JointType_Head, JointType_Neck);
	skeletonDrawOrder[1] = make_pair<JointType, JointType>(JointType_Neck, JointType_SpineShoulder);
	skeletonDrawOrder[2] = make_pair<JointType, JointType>(JointType_SpineShoulder, JointType_SpineMid);
	skeletonDrawOrder[3] = make_pair<JointType, JointType>(JointType_SpineMid, JointType_SpineBase);
	skeletonDrawOrder[4] = make_pair<JointType, JointType>(JointType_SpineShoulder, JointType_ShoulderRight);
	skeletonDrawOrder[5] = make_pair<JointType, JointType>(JointType_SpineShoulder, JointType_ShoulderLeft);
	skeletonDrawOrder[6] = make_pair<JointType, JointType>(JointType_SpineBase, JointType_HipRight);
	skeletonDrawOrder[7] = make_pair<JointType, JointType>(JointType_SpineBase, JointType_HipLeft);

	// Right Arm    
	skeletonDrawOrder[8] = make_pair<JointType, JointType>(JointType_ShoulderRight, JointType_ElbowRight);
	skeletonDrawOrder[9] = make_pair<JointType, JointType>(JointType_ElbowRight, JointType_WristRight);
	skeletonDrawOrder[10] = make_pair<JointType, JointType>(JointType_WristRight, JointType_HandRight);
	skeletonDrawOrder[11] = make_pair<JointType, JointType>(JointType_HandRight, JointType_HandTipRight);
	skeletonDrawOrder[12] = make_pair<JointType, JointType>(JointType_WristRight, JointType_ThumbRight);

	// Left Arm
	skeletonDrawOrder[13] = make_pair<JointType, JointType>(JointType_ShoulderLeft, JointType_ElbowLeft);
	skeletonDrawOrder[14] = make_pair<JointType, JointType>(JointType_ElbowLeft, JointType_WristLeft);
	skeletonDrawOrder[15] = make_pair<JointType, JointType>(JointType_WristLeft, JointType_HandLeft);
	skeletonDrawOrder[16] = make_pair<JointType, JointType>(JointType_HandLeft, JointType_HandTipLeft);
	skeletonDrawOrder[17] = make_pair<JointType, JointType>(JointType_WristLeft, JointType_ThumbLeft);

	// Right Leg
	skeletonDrawOrder[18] = make_pair<JointType, JointType>(JointType_HipRight, JointType_KneeRight);
	skeletonDrawOrder[19] = make_pair<JointType, JointType>(JointType_KneeRight, JointType_AnkleRight);
	skeletonDrawOrder[20] = make_pair<JointType, JointType>(JointType_AnkleRight, JointType_FootRight);

	// Left Leg
	skeletonDrawOrder[21] = make_pair<JointType, JointType>(JointType_HipLeft, JointType_KneeLeft);
	skeletonDrawOrder[22] = make_pair<JointType, JointType>(JointType_KneeLeft, JointType_AnkleLeft);
	skeletonDrawOrder[23] = make_pair<JointType, JointType>(JointType_AnkleLeft, JointType_FootLeft);

}

ofxKinectCommonBridge::~ofxKinectCommonBridge(){
	stop();
}


//---------------------------------------------------------------------------
void ofxKinectCommonBridge::setDepthClipping(float nearClip, float farClip){
	nearClipping = nearClip;
	farClipping = farClip;
	updateDepthLookupTable();
}

//---------------------------------------------------------------------------
void ofxKinectCommonBridge::updateDepthLookupTable()
{
	unsigned char nearColor = bNearWhite ? 255 : 0;
	unsigned char farColor = bNearWhite ? 0 : 255;
	unsigned int maxDepthLevels = 10001;
	depthLookupTable.resize(maxDepthLevels);
	depthLookupTable[0] = 0;
	for(unsigned int i = 1; i < maxDepthLevels; i++)
	{
		depthLookupTable[i] = ofMap(i, nearClipping, farClipping, nearColor, farColor, true);
	}
}

/// is the current frame new?
bool ofxKinectCommonBridge::isFrameNew(){
	return isFrameNewVideo() || isFrameNewDepth();
}

bool ofxKinectCommonBridge::isFrameNewVideo(){
	return bIsFrameNewVideo;
}

bool ofxKinectCommonBridge::isFrameNewDepth(){
	return bIsFrameNewDepth;
}

bool ofxKinectCommonBridge::isNewSkeleton() {
	return bNeedsUpdateSkeleton;
}

void ofxKinectCommonBridge::checkOpenGLError(string function){
    GLuint err = glGetError();
    if (err != GL_NO_ERROR){
        //ofLogError( "CloudsVisualSystem::checkOpenGLErrors") << "OpenGL generated error " << ofToString(err) << " : " << gluErrorString(err) << " in " << function;
    }
}

int ofxKinectCommonBridge::getGLFormat(KCBColorImageFormat format){
	switch(format){
	case KCBColorImageFormat_Rgba:
		return GL_RGBA;
	case KCBColorImageFormat_Bgra:
		return GL_BGRA;
	case KCBColorImageFormat_Yuy2:
		return GL_RG16;
	}
}

int ofxKinectCommonBridge::getGLFormat(ColorImageFormat format){
	switch(format){
	case ColorImageFormat_Rgba:
		return GL_RGBA;
	case ColorImageFormat_Bgra:
		return GL_BGRA;
	case ColorImageFormat_Yuy2:
		return GL_RG16;
	default:
		ofLogError("ofxKinectCommonBridge::getGLFormat(ColorImageFormat)") << " Color source is of unsupported format";
		return GL_LUMINANCE;
	}
}

ColorImageFormat ofxKinectCommonBridge::getColorImageFormat(KCBColorImageFormat format){
	switch(format){
	case KCBColorImageFormat_Rgba:
		return ColorImageFormat_Rgba;
	case KCBColorImageFormat_Bgra:
		return ColorImageFormat_Bgra;
	case KCBColorImageFormat_Yuy2:
		return ColorImageFormat_Yuy2;
	}
}

/// updates the pixel buffers and textures
/// make sure to call this to update to the latest incoming frames
void ofxKinectCommonBridge::update()
{

	checkOpenGLError("KCB:: UPDATE BEGAN");

	if(!bStarted)
	{
		ofLogError("ofxKinectCommonBridge::update") << "Kinect not started";
		return;
	}

	// update color or IR pixels and textures if necessary
	if(bNeedsUpdateVideo)
	{
		bIsFrameNewVideo = true;
		bNeedsUpdateVideo = false;

		//swap(videoPixelsBack, videoPixels);
		swap(pColorFrame, pColorFrameBack);

		if(bUseTexture) {
			if(bVideoIsInfrared)
			{
				if(bProgrammableRenderer){
					videoTex.loadData(pColorFrame->Buffer, colorFrameDescription.width, colorFrameDescription.height, GL_RED);
				} else {
					videoTex.loadData(pColorFrame->Buffer, colorFrameDescription.width, colorFrameDescription.height, GL_LUMINANCE16);
				}
			}
			else if(bVideoIsColor)
			{
				videoTex.loadData(pColorFrame->Buffer, colorFrameDescription.width, colorFrameDescription.height, getGLFormat(pColorFrame->Format));
			}
		}
	} else {
		bIsFrameNewVideo = false;
	}

	checkOpenGLError("KCB:: VIDEO");

	// update depth pixels and texture if necessary
	if(bNeedsUpdateDepth)
	{

		//swap(depthPixelsRawBack, depthPixelsRaw);
		swap(pDepthFrameBack, pDepthFrame);

		if(mappingColorToDepth) {
			beginMappingColorToDepth = true;
		}

		bIsFrameNewDepth = true;
		bNeedsUpdateDepth = false;

		for(int i = 0; i < depthPixels.getWidth()*depthPixels.getHeight(); i++) {
			depthPixelsRaw.getPixels()[i] = pDepthFrame->Buffer[i];
			depthPixels.getPixels()[i]    = depthLookupTable[ofClamp(depthPixelsRaw.getPixels()[i], 0, depthLookupTable.size() - 1)];
			if(bUseFloatTexture){
				depthPixelsNormalized.getPixels()[i] =  depthPixelsRaw.getPixels()[i] / 65535.0f;
			}
		}

		if(bUseTexture) {
			if( bProgrammableRenderer ) {
				depthTex.loadData(depthPixels.getPixels(), depthFrameDescription.width, depthFrameDescription.height, GL_RED);
				checkOpenGLError("KCB:: BEFORE LOAD DEPTH");
				if(bUseFloatTexture){
					rawDepthTex.loadData(depthPixelsNormalized.getPixels(),depthFrameDescription.width, depthFrameDescription.height, GL_RED);
				}
				else{			
					rawDepthTex.loadData(depthPixelsRaw.getPixels(), depthFrameDescription.width, depthFrameDescription.height, GL_LUMINANCE_INTEGER_EXT );
				}
				//FAILED ALTERNATIVES
				//rawDepthTex.loadData(depthPixelsRaw.getPixels(), depthFrameDescription.width, depthFrameDescription.height, GL_RED);
				//rawDepthTex.loadData(depthPixelsRaw.getPixels(), depthFrameDescription.width, depthFrameDescription.height, GL_RED_INTEGER);
				checkOpenGLError("KCB:: AFTER LOAD DEPTH");
			} else {
				depthTex.loadData(depthPixels.getPixels(), depthFrameDescription.width, depthFrameDescription.height, GL_LUMINANCE);
				rawDepthTex.loadData(pDepthFrame->Buffer, depthFrameDescription.width, depthFrameDescription.height, GL_LUMINANCE16);
			}
		}
	} else {
		bIsFrameNewDepth = false;
	}

	checkOpenGLError("KCB:: DEPTH");

	// update skeletons if necessary
	if(bUsingSkeletons && bNeedsUpdateSkeleton)
	{	
		swap(skeletons, backSkeletons);
		bNeedsUpdateSkeleton = false;
	} 

	if (bNeedsUpdateBodyIndex)
	{
		
		swap(pBodyIndexFrame, pBodyIndexFrame);

		if (bProgrammableRenderer)
		{
			bodyIndexTex.loadData(pBodyIndexFrame->Buffer, bodyIndexFrameDescription.width, bodyIndexFrameDescription.height, GL_RED);
		}
		else
		{
			bodyIndexTex.loadData(pBodyIndexFrame->Buffer, bodyIndexFrameDescription.width, bodyIndexFrameDescription.height, GL_LUMINANCE);
		}

		bNeedsUpdateBodyIndex = false;
	}

	checkOpenGLError("KCB:: SKELETON");
}

//------------------------------------
ofPixels& ofxKinectCommonBridge::getColorPixelsRef(){
	return videoPixels;
}

//------------------------------------
ofPixels & ofxKinectCommonBridge::getDepthPixelsRef(){       	///< grayscale values
	return depthPixels;
}

//------------------------------------
ofShortPixels & ofxKinectCommonBridge::getRawDepthPixelsRef(){
	return depthPixelsRaw;
}

//------------------------------------
void ofxKinectCommonBridge::setUseTexture(bool bUse){
	bUseTexture = bUse;
}

void ofxKinectCommonBridge::setRawTextureUsesFloats(bool bUseRawFloat){
	bUseFloatTexture = true;
}

//----------------------------------------------------------
void ofxKinectCommonBridge::draw(float _x, float _y, float _w, float _h) {
	if(bUseTexture) {
		videoTex.draw(_x, _y, _w, _h);
	}
}

//----------------------------------------------------------
void ofxKinectCommonBridge::draw(float _x, float _y) {
	draw(_x, _y, colorFrameDescription.width, colorFrameDescription.height);
}

//----------------------------------------------------------
void ofxKinectCommonBridge::draw(const ofPoint & point) {
	draw(point.x, point.y);
}

//----------------------------------------------------------
void ofxKinectCommonBridge::draw(const ofRectangle & rect) {
	draw(rect.x, rect.y, rect.width, rect.height);
}

//----------------------------------------------------------
void ofxKinectCommonBridge::drawRawDepth(float _x, float _y, float _w, float _h) {
	if(bUseTexture) {
		rawDepthTex.draw(_x, _y, _w, _h);
	}
}

//----------------------------------------------------------
void ofxKinectCommonBridge::drawRawDepth(float _x, float _y) {
	drawRawDepth(_x, _y, depthFrameDescription.width, depthFrameDescription.height);
}

//----------------------------------------------------------
void ofxKinectCommonBridge::drawRawDepth(const ofPoint & point) {
	drawRawDepth(point.x, point.y);
}

//----------------------------------------------------------
void ofxKinectCommonBridge::drawRawDepth(const ofRectangle & rect) {
	drawRawDepth(rect.x, rect.y, rect.width, rect.height);
}

//----------------------------------------------------------
void ofxKinectCommonBridge::drawDepth(float _x, float _y, float _w, float _h) {
	if(bUseTexture) {
		depthTex.draw(_x, _y, _w, _h);
	}
}

//---------------------------------------------------------------------------
void ofxKinectCommonBridge::drawDepth(float _x, float _y) {
	drawDepth(_x, _y, (float) depthFrameDescription.width, (float) depthFrameDescription.height);
}

//----------------------------------------------------------
void ofxKinectCommonBridge::drawDepth(const ofPoint & point) {
	drawDepth(point.x, point.y);
}

//----------------------------------------------------------
void ofxKinectCommonBridge::drawDepth(const ofRectangle & rect) {
	drawDepth(rect.x, rect.y, rect.width, rect.height);
}

//----------------------------------------------------------
void ofxKinectCommonBridge::drawBodyIndex(float x, float y) {
	bodyIndexTex.draw(x, y);
}

void ofxKinectCommonBridge::drawAllSkeletons()
{
	drawAllSkeletons(ofVec2f(depthFrameDescription.width, depthFrameDescription.height) );
}

void ofxKinectCommonBridge::drawAllSkeletons(ofVec2f scale)
{
	for (int i = 0; i < skeletons.size(); i++)
	{
		if (skeletons[i].tracked)
		{
			drawSkeleton(i, scale);
		}
	}
}

//
void ofxKinectCommonBridge::drawSkeleton( int index)
{
	drawSkeleton(index, ofVec2f(depthFrameDescription.width, depthFrameDescription.height) );
}

void ofxKinectCommonBridge::drawSkeleton( int index, ofVec2f scale )
{
	if(index >= skeletons.size())
	{
		ofLog() << " skeleton index too high " << endl;
		return;
	}

	if (!skeletons[index].tracked)
	{
		ofLog() << " skeleton index not being tracked " << endl;
		return;
	}

	ofVec3f lastPosition(-1,-1,-1);
	ofVec3f normalize(0.5, 0.5, 0);
	ofVec3f scale3(scale.x, scale.y, 1);

	// Iterate through joints
	//for (map<JointType, Kv2Joint>::iterator it = skeletons[index].joints.begin(); it != skeletons[index].joints.end(); ++it)
	//{

	//	//// Get position and rotation
	//	Kv2Joint joint = it->second;

	//	ofSetColor(255, 0, 0);
	//	ofVec3f screenPosition = (joint.getPosition() + normalize) * scale3;
	//	screenPosition.y = 480 - screenPosition.y;
	//	ofCircle(screenPosition, 10);

	//	lastPosition = joint.getPosition() * scale;
	//}

	for (int i = 0; i < JointType_Count; i++)
	{
		ofSetLineWidth(2);

		ofVec3f lineBegin = (skeletons[index].joints[skeletonDrawOrder[i].first].getPosition() + normalize) * scale3;
		lineBegin.y = scale3.y - lineBegin.y;
		ofVec3f lineEnd = (skeletons[index].joints[skeletonDrawOrder[i].second].getPosition() + normalize) * scale3;
		lineEnd.y = scale3.y - lineEnd.y;

		ofSetColor(0, 255, 0);
		ofLine(lineBegin, lineEnd);
		ofSetColor(255, 0, 0);
		ofCircle(lineEnd, 10);
	}

	ofSetColor(255, 255, 255);
}


bool ofxKinectCommonBridge::initSensor( int id )
{
	if(bStarted){
		ofLogError("ofxKinectCommonBridge::initSensor") << "Cannot configure once the sensor has already started" << endl;
		return false;
	}
	if (ofGetCurrentRenderer()->getType() == ofGLProgrammableRenderer::TYPE)
	{
		bProgrammableRenderer = true;
	}

	hKinect = KCBOpenDefaultSensor();

	return true;
}

bool ofxKinectCommonBridge::initDepthStream( bool mapDepthToColor )
{

	mappingDepthToColor = mapDepthToColor;

	if(bStarted){
		ofLogError("ofxKinectCommonBridge::initDepthStream") << " Cannot configure once the sensor has already started";
		return false;
	}

	HRESULT hr;
	hr = KCBGetDepthFrameDescription(hKinect, &depthFrameDescription);

	//hr = KCBCreateDepthFrame(depthFrameDescription, &pDepthFrame);

	if(bProgrammableRenderer) {
		depthPixels.allocate(depthFrameDescription.width, depthFrameDescription.height, OF_IMAGE_COLOR);
		depthPixelsBack.allocate(depthFrameDescription.width, depthFrameDescription.height, OF_IMAGE_COLOR);
	} else {
		depthPixels.allocate(depthFrameDescription.width, depthFrameDescription.height, OF_IMAGE_GRAYSCALE);
		depthPixelsBack.allocate(depthFrameDescription.width, depthFrameDescription.height, OF_IMAGE_GRAYSCALE);
	}

	depthPixelsRawFront.allocate(depthFrameDescription.width, depthFrameDescription.height, OF_IMAGE_GRAYSCALE);
	depthPixelsRawBack.allocate(depthFrameDescription.width, depthFrameDescription.height, OF_IMAGE_GRAYSCALE);
	depthPixelsRaw.allocate(depthFrameDescription.width, depthFrameDescription.height, OF_IMAGE_GRAYSCALE);
	depthPixelsNormalized.allocate(depthFrameDescription.width, depthFrameDescription.height, OF_IMAGE_GRAYSCALE);

	pDepthFrame = new KCBDepthFrame();
	pDepthFrame->Buffer = depthPixelsRawFront.getPixels();
	pDepthFrame->Size = depthFrameDescription.lengthInPixels;

	pDepthFrameBack = new KCBDepthFrame();
	pDepthFrameBack->Buffer = depthPixelsRawBack.getPixels();
	pDepthFrameBack->Size = depthFrameDescription.lengthInPixels;

	if(bUseTexture){

		if(bProgrammableRenderer) {
			//int w, int h, int glInternalFormat, bool bUseARBExtention, int glFormat, int pixelType
			depthTex.allocate(depthFrameDescription.width, depthFrameDescription.height, GL_R8);//, true, GL_R8, GL_UNSIGNED_BYTE);
			depthTex.setRGToRGBASwizzles(true);

			checkOpenGLError("ERROR ALLOCATING NORMAL DEPTH");
			if(bUseFloatTexture){
				//WORKS ON ALL CARDS SO FAR
				rawDepthTex.allocate(depthFrameDescription.width, depthFrameDescription.height, GL_R32F);
			}
			else{
				//WORKS SOMETIMES BUT FAILS ON SOME CARDS (Iris 5200)
				//shader then requires 
				//	#extension GL_EXT_gpu_shader4 : enable  
				//	and sampler to be usampler2D
				rawDepthTex.allocate(depthFrameDescription.width, depthFrameDescription.height, GL_LUMINANCE16UI_EXT, true, GL_LUMINANCE_INTEGER_EXT, GL_UNSIGNED_SHORT);
			}
			checkOpenGLError("ERROR ALLOCATING RAW DEPTH");

			//failed alternatives....
			//WORKS BUT IS CLAMPED
			//rawDepthTex.allocate(depthFrameDescription.width, depthFrameDescription.height, GL_R16, true, GL_RED, GL_UNSIGNED_SHORT);
			//THROWS NO ERRORS BUT CANT BE DRAWN
			//rawDepthTex.allocate(depthFrameDescription.width, depthFrameDescription.height, GL_R16UI, true, GL_RED_INTEGER , GL_UNSIGNED_SHORT);
			//depthTex.setRGToRGBASwizzles(true);			

			//cout << rawDepthTex.getWidth() << " " << rawDepthTex.getHeight() << endl;
			//depthTex.allocate(K2_IR_WIDTH, K2_IR_HEIGHT, GL_RGB);
		} else {

			depthTex.allocate(depthFrameDescription.width, depthFrameDescription.height, GL_LUMINANCE);
			rawDepthTex.allocate(depthFrameDescription.width, depthFrameDescription.height, GL_LUMINANCE16);

	
		}
	}
	
	bUsingDepth = true;
	return bInited;
}

bool ofxKinectCommonBridge::initColorStream( bool mapColorToDepth, KCBColorImageFormat format)
{

	KCBGetColorFrameDescription(hKinect, ColorImageFormat_Rgba, &colorFrameDescription);

	if(bUseTexture){
		if (format == KCBColorImageFormat_Yuy2 && !bProgrammableRenderer){
			ofLogError("ofxKinectCommonBridge::initColorStream", "yuy2 needs programmable renderer");
		} else {
			videoTex.allocate(colorFrameDescription.width, colorFrameDescription.height, getGLFormat(format));
		}
	}

	if (format == KCBColorImageFormat_Yuy2)
	{
		videoPixels.allocate(colorFrameDescription.width, colorFrameDescription.height, 2);
		videoPixelsBack.allocate(colorFrameDescription.width, colorFrameDescription.height, 2);
	}
	else
	{
		videoPixels.allocate(colorFrameDescription.width, colorFrameDescription.height, OF_IMAGE_COLOR_ALPHA);
		videoPixelsBack.allocate(colorFrameDescription.width, colorFrameDescription.height, OF_IMAGE_COLOR_ALPHA);
	}

	pColorFrame = new KCBColorFrame();
	pColorFrame->Buffer = videoPixels.getPixels();
	pColorFrame->Size = colorFrameDescription.lengthInPixels * colorFrameDescription.bytesPerPixel;
	pColorFrame->Format = getColorImageFormat(format);


	pColorFrameBack = new KCBColorFrame();
	pColorFrameBack->Buffer = videoPixelsBack.getPixels();
	pColorFrameBack->Size = colorFrameDescription.lengthInPixels * colorFrameDescription.bytesPerPixel;
	pColorFrameBack->Format = getColorImageFormat(format);

	bVideoIsColor = true;
	bVideoIsInfrared = false;

	//HRESULT hr = KCBCreateColorFrame(ColorImageFormat_Rgba, colorFrameDescription, &pColorFrame);
	return true;
}

bool ofxKinectCommonBridge::initIRStream( int width, int height )
{
	if(bStarted){
		ofLogError("ofxKinectCommonBridge::startIRStream") << " Cannot configure when the sensor has already started";
		return false;
	}

	bVideoIsInfrared = true;
	bVideoIsColor = false;

	KCBGetInfraredFrameDescription(hKinect, &irFrameDescription);

	irPixelsRaw.allocate(irFrameDescription.width, irFrameDescription.height, OF_IMAGE_GRAYSCALE);

	pInfraredFrameBack = new KCBInfraredFrame();
	pInfraredFrameBack->Buffer = irPixelsBackRaw.getPixels();
	pInfraredFrameBack->Size = irFrameDescription.lengthInPixels;

	pInfraredFrame = new KCBInfraredFrame();
	pInfraredFrame->Buffer = irPixelsRaw.getPixels();
	pInfraredFrame->Size = irFrameDescription.lengthInPixels;

	if(bUseTexture)
	{
		if(bProgrammableRenderer){
			videoTex.allocate(irFrameDescription.width, irFrameDescription.height, GL_R8);
			videoTex.setRGToRGBASwizzles(true);
		}
		else{
			videoTex.allocate(irFrameDescription.width, irFrameDescription.height, GL_LUMINANCE);
		}
	}

	bInited = true;
	ofLogError("ofxKinectCommonBridge::initIRStream") << "cannot initialize stream";
	return true;
}

bool ofxKinectCommonBridge::initBodyIndexStream()
{
	if (bStarted){
		ofLogError("ofxKinectCommonBridge::initBodyIndexStream") << "Cannot configure once the sensor has already started";
		return false;
	}
	HRESULT hr = KCBGetBodyIndexFrameDescription(hKinect, &bodyIndexFrameDescription);

	if (!SUCCEEDED(hr))
	{
		ofLogError("ofxKinectCommonBridge::initBodyIndexStream") << "cannot initialize stream";
		return false;
	}

	bodyIndexPixels.allocate(bodyIndexFrameDescription.width, bodyIndexFrameDescription.height, OF_IMAGE_GRAYSCALE);
	bodyIndexPixelsBack.allocate(bodyIndexFrameDescription.width, bodyIndexFrameDescription.height, OF_IMAGE_GRAYSCALE);

	pBodyIndexFrameBack = new KCBBodyIndexFrame();
	pBodyIndexFrameBack->Buffer = bodyIndexPixelsBack.getPixels();
	pBodyIndexFrameBack->Size = bodyIndexFrameDescription.lengthInPixels;

	pBodyIndexFrame = new KCBBodyIndexFrame();
	pBodyIndexFrame->Buffer = bodyIndexPixels.getPixels();
	pBodyIndexFrame->Size = bodyIndexFrameDescription.lengthInPixels;

	if (bProgrammableRenderer)
	{
		bodyIndexTex.allocate(bodyIndexFrameDescription.width, bodyIndexFrameDescription.height, GL_R8);
	}
	else
	{
		bodyIndexTex.allocate(bodyIndexFrameDescription.width, bodyIndexFrameDescription.height, GL_LUMINANCE);
	}
	bUsingBodyIndex = true;

	return true; //??
}

bool ofxKinectCommonBridge::initSkeletonStream( bool seated )
{
	if(bStarted){
		ofLogError("ofxKinectCommonBridge::initSkeletonStream") << "Cannot configure once the sensor has already started";
		return false;
	}

	skeletons.resize(6);
	backSkeletons.resize(6);

	//HRESULT hr = KCBCreateBodyFrame(&pBodyFrame);
	//if (hr >= 0){
		bUsingSkeletons = true;
		return true;
	//}
	//ofLogError("ofxKinectCommonBridge::initSkeletonStream") << "cannot initialize stream";
	//return false;
}

//----------------------------------------------------------
bool ofxKinectCommonBridge::start()
{
	startThread(true, false);
	bStarted = true;	
	return true;
}

ofVec3f ofxKinectCommonBridge::mapDepthToSkeleton(ofPoint depthPoint){
	vector<ofPoint> pt;
	pt.push_back(depthPoint);
	return mapDepthToSkeleton(pt)[0];
}

//----------------------------------------------------------
ofVec3f ofxKinectCommonBridge::mapDepthToSkeleton(ofPoint depthPoint, ofShortPixels& depthImage){
	vector<ofPoint> pt;
	pt.push_back(depthPoint);
	return mapDepthToSkeleton(pt,depthImage)[0];
}

//----------------------------------------------------------
vector<ofVec3f> ofxKinectCommonBridge::mapDepthToSkeleton(vector<ofPoint>& depthPoints){
	return mapDepthToSkeleton(depthPoints, getRawDepthPixelsRef());
}

//----------------------------------------------------------
vector<ofVec3f> ofxKinectCommonBridge::mapDepthToSkeleton(vector<ofPoint>& depthPoints, ofShortPixels& depthImage){
	vector<DepthSpacePoint> depthPixels;
	vector<UINT16> depths;
	vector<CameraSpacePoint> cameraPoints;
	
	int depthArraySize = depthFrameDescription.width * depthFrameDescription.height;
	depthPixels.resize(depthArraySize);
	depths.resize(depthArraySize);
	cameraPoints.resize(depthArraySize);

	for(int y = 0; y < depthFrameDescription.height; y++){
		for(int x = 0; x < depthFrameDescription.width; x++) {
			int i = y*depthFrameDescription.width+x;
			depths[i] = (UINT16)depthImage.getPixels()[i];
			depthPixels[i].X = x;
			depthPixels[i].Y = y;
		}
	}

	HRESULT mapResult;
	mapResult = KCBMapDepthPointsToCameraSpace(hKinect, 
		depthArraySize, &depthPixels[0],
		depthArraySize, &depths[0],
		depthArraySize, &cameraPoints[0]);
	
	vector<ofVec3f> points;
	for(int i = 0; i < depthPoints.size(); i++){
		CameraSpacePoint& p = cameraPoints[int(depthPoints[i].y) * depthFrameDescription.width + int(depthPoints[i].x)]; 
		points.push_back( ofVec3f(p.X,p.Y,p.Z) );
	}
	return points;
}

/*
//----------------------------------------------------------
ofVec2f ofxKinectCommonBridge::mapColorToDepth(ofPoint colorPoint){
	vector<ofPoint> pt;
	pt.push_back(colorPoint);
	return mapColorToDepth(pt)[0];
}

//----------------------------------------------------------
ofVec2f ofxKinectCommonBridge::mapColorToDepth(ofPoint colorPoint, ofShortPixels& depthImage){
	vector<ofPoint> pt;
	pt.push_back(colorPoint);
	return mapColorToDepth(pt,depthImage)[0];
}

//----------------------------------------------------------
vector<ofVec2f> ofxKinectCommonBridge::mapColorToDepth(vector<ofPoint>& colorPoints){
	return mapColorToDepth(colorPoints, getRawDepthPixelsRef());
}

//----------------------------------------------------------
vector<ofVec2f> ofxKinectCommonBridge::mapColorToDepth(vector<ofPoint>& colorPoints, ofShortPixels& depthImage){
	vector<NUI_DEPTH_IMAGE_PIXEL> depthPixels;
	vector<NUI_DEPTH_IMAGE_POINT> depthPoints;

	int depthArraySize = depthFormat.dwHeight*depthFormat.dwWidth;
	int colorArraySize = colorFormat.dwHeight*colorFormat.dwWidth;

	depthPixels.resize(depthArraySize);
	depthPoints.resize(colorArraySize);

	for(int i = 0; i < depthImage.getWidth()*depthImage.getHeight(); i++) {
		depthPixels[i].depth = (USHORT)depthImage.getPixels()[i];
		depthPixels[i].playerIndex = 0;
	}

	HRESULT mapResult;
	mapResult = KinectMapColorFrameToDepthFrame(
		hKinect, NUI_IMAGE_TYPE_COLOR,
		colorRes, depthRes,
		depthArraySize, &depthPixels[0],
		colorArraySize, &depthPoints[0]);

	vector<ofVec2f> points;
	for(int i = 0; i < colorPoints.size(); i++){
		NUI_DEPTH_IMAGE_POINT& p = depthPoints[ int(colorPoints[i].y)*colorFormat.dwWidth+int(colorPoints[i].x) ]; 
		points.push_back( ofVec2f(p.x,p.y) );
	}
	
	return points;
}

//----------------------------------------------------------
ofVec3f ofxKinectCommonBridge::mapColorToSkeleton(ofPoint colorPoint){
	vector<ofPoint> pt;
	pt.push_back(colorPoint);
	return mapColorToSkeleton(pt)[0];

}

//----------------------------------------------------------
ofVec3f ofxKinectCommonBridge::mapColorToSkeleton(ofPoint colorPoint, ofShortPixels& depthImage){
	vector<ofPoint> pt;
	pt.push_back(colorPoint);
	return mapColorToSkeleton(pt,depthImage)[0];
}

//----------------------------------------------------------
vector<ofVec3f> ofxKinectCommonBridge::mapColorToSkeleton(vector<ofPoint>& colorPoints){
	return mapColorToSkeleton(colorPoints, getRawDepthPixelsRef());
}

//----------------------------------------------------------
vector<ofVec3f> ofxKinectCommonBridge::mapColorToSkeleton(vector<ofPoint>& colorPoints, ofShortPixels& depthImage){

	vector<NUI_DEPTH_IMAGE_PIXEL> depthPixels;
	vector<Vector4> depthPoints;
	int depthArraySize = depthFormat.dwHeight*depthFormat.dwWidth;
	int colorArraySize = colorFormat.dwHeight*colorFormat.dwWidth;

	depthPixels.resize(depthArraySize);
	depthPoints.resize(colorArraySize);

	for(int i = 0; i < depthImage.getWidth()*depthImage.getHeight(); i++) {
		depthPixels[i].depth = (USHORT)depthImage.getPixels()[i];
		depthPixels[i].playerIndex = 0;
	}

	HRESULT mapResult;
	mapResult = KinectMapColorFrameToSkeletonFrame(
		hKinect, NUI_IMAGE_TYPE_COLOR,
		colorRes, depthRes,
		depthArraySize, &depthPixels[0],
		colorArraySize, &depthPoints[0]);

	vector<ofVec3f> points;
	for(int i = 0; i < colorPoints.size(); i++){
		Vector4 pos = depthPoints[int(colorPoints[i].y)*colorFormat.dwWidth+int(colorPoints[i].x)];
		points.push_back( ofVec3f(pos.x,pos.y,pos.z) );
	}
	return points;
}
*/

//----------------------------------------------------------
void ofxKinectCommonBridge::stop() {
	if(bStarted){
		
		waitForThread(true);

		bStarted = false;

		KCBCloseSensor(&hKinect);

		//delete pBodies; //KCBReleaseBodyFrame(&pBodyFrame);
		KCBReleaseBodyIndexFrame(&pBodyIndexFrame);

		KCBReleaseColorFrame(&pColorFrame);
		KCBReleaseColorFrame(&pColorFrameBack);

		KCBReleaseDepthFrame(&pDepthFrame);
		KCBReleaseDepthFrame(&pDepthFrameBack);

		KCBReleaseInfraredFrame(&pInfraredFrame);
		KCBReleaseInfraredFrame(&pInfraredFrameBack);

		//KCBReleaseLongExposureInfraredFrame(_Inout_ KCBLongExposureInfraredFrame** pLongExposureInfraredFrame);

	}
}	

//----------------------------------------------------------
void ofxKinectCommonBridge::threadedFunction(){

	LONGLONG timestamp;
	
	//how can we tell?
	while(isThreadRunning()) {

		// KCBAllFramesReady
		//lock();
			if (bUsingDepth && SUCCEEDED(KCBGetDepthFrame(hKinect, pDepthFrame)))
			{
				bNeedsUpdateDepth = true;
			}
		//unlock();

		if(bUsingSkeletons) 
		{
			LONGLONG timestamp;
			IBodyFrame* pBodyFrame = NULL;
			IBody* ppBodies[BODY_COUNT] = { 0 };
			if (SUCCEEDED(KCBGetIBodyFrame(hKinect, &pBodyFrame)))
			{
				HRESULT hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, ppBodies);
				bNeedsUpdateSkeleton = true;

				// buffer for later
				for (int i = 0; i < BODY_COUNT; ++i)
				{
					backSkeletons[i].joints.clear();
					backSkeletons[i].tracked = false;

					IBody *pBody = ppBodies[i];
					BOOLEAN isTracked = false;

					if (pBody == NULL)
					{
						continue;
					}

					HRESULT hr = pBody->get_IsTracked(&isTracked);
					if (isTracked)
					{
						HRESULT hrJoints = ppBodies[i]->GetJoints(JointType_Count, joints);
						HRESULT hrOrient = ppBodies[i]->GetJointOrientations(JointType_Count, jointOrients);
						if (FAILED(hrJoints))
						{
							ofLogError("ofxKinectCommonBridge::threadedFunction") << "Failed to get joints";
						}

						if (FAILED(hrOrient))
						{
							ofLogError("ofxKinectCommonBridge::threadedFunction") << "Failed to get orientations";
						}

						if (SUCCEEDED(hrJoints) && SUCCEEDED(hrOrient))
						{
							for (int j = 0; j < JointType_Count; ++j)
							{
								backSkeletons[i].joints[joints[j].JointType] = Kv2Joint(joints[j], jointOrients[j]);
							}
						}
						backSkeletons[i].tracked = true;
					}
					pBody->Release();
				}

				// all done clean up
				pBodyFrame->Release();
			}
		}

		if (bUsingBodyIndex)
		{
			
			if (SUCCEEDED(KCBGetBodyIndexFrame(hKinect, pBodyIndexFrame)))
			{
				bNeedsUpdateBodyIndex = true;
			}
		}

		if (bVideoIsInfrared)
		{
			if (SUCCEEDED(KCBGetInfraredFrame(hKinect, pInfraredFrame)))
			{
				bNeedsUpdateVideo = true;
				// do we need to do this anymore?
				for (int i = 0; i <colorFrameDescription.width * colorFrameDescription.height; i++)
				{
					videoPixels.getPixels()[i] = reinterpret_cast<USHORT*>(irPixelByteArray)[i] >> 8;
				}
			}
		}
		else if(bVideoIsColor)
		{
			if (SUCCEEDED(KCBGetColorFrame(hKinect, pColorFrame)))
			{
				bNeedsUpdateVideo = true;
			}
		}

		//TODO: TILT
		//TODO: ACCEL
		//TODO: FACE
		//TODO: AUDIO
		ofSleepMillis(10);
	}
}
