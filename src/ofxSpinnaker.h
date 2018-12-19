#pragma once

#include "ofMain.h"

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"

static const int DEFAULT_CAM_W = 720;
static const int DEFAULT_CAM_H = 540;

//class ofxSpinnaker {
class ofxSpinnaker : public ofThread {

public:
	ofxSpinnaker();
	~ofxSpinnaker();

	void start();
	void stop();
	void threadedFunction();

	//basic function----------
	void setup();
	void setup(int width, int height, int offset_x = 0, int offset_y = 0);
	void exit();

	void draw(int x, int y);

	ofImage getOfImage() { return srcImage; }

	// getter
	int getImageWidth() { return width; }
	int getImageHeight() { return height; }

	// setter
	void setFrameRate(float fps);
	void setExposure(float exposure_ms);

private:

	void update();

	int width;
	int height;

	//cv::Mat src_mat;

	// spinnader params
	Spinnaker::SystemPtr system;
	Spinnaker::CameraList camList;
	Spinnaker::CameraPtr pCam;
	bool isCameraActive;

	int PrintDeviceInfo(Spinnaker::GenApi::INodeMap & nodeMap);
	void grabNewImage();

	ofImage srcImage;
	unsigned char *imageBuffer;

	uint64_t startTime;
};
