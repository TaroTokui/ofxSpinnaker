#include "ofxSpinnaker.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

//--------------------------------------------------------------
void ofxSpinnaker::start()
{
	startThread();
}
//--------------------------------------------------------------
void ofxSpinnaker::stop()
{
	stopThread();
}

//--------------------------------------------------------------
void ofxSpinnaker::threadedFunction()
{
	while (isThreadRunning())
	{
		// Attempt to lock the mutex.  If blocking is turned on,
		if (lock())
		{
			update();
			unlock();
		}
		ofSleepMillis(1);
		//cout << ofGetElapsedTimeMillis() - startTime << endl;
		//startTime = ofGetElapsedTimeMillis();
	}
}

//--------------------------------------------------------------
ofxSpinnaker::ofxSpinnaker()
{
	// Retrieve singleton reference to system object
	system = System::GetInstance();

	// Retrieve list of cameras from the system
	camList = system->GetCameras();

	pCam = NULL;

}

//--------------------------------------------------------------
ofxSpinnaker::~ofxSpinnaker()
{
	delete imageBuffer;
	pCam = NULL;

	// Clear camera list before releasing system
	camList.Clear();
}

//--------------------------------------------------------------
void ofxSpinnaker::setup()
{
	setup(DEFAULT_CAM_W, DEFAULT_CAM_H, 0, 0);
}

//--------------------------------------------------------------
void ofxSpinnaker::setup(int _width, int _height, int _offset_x, int _offset_y)
{
	width = _width;
	height = _height;

	int result = 0;
	isCameraActive = false;

	// Print application build information
	cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

	// Print out current library version
	const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
	cout << "Spinnaker library version: "
		<< spinnakerLibraryVersion.major << "."
		<< spinnakerLibraryVersion.minor << "."
		<< spinnakerLibraryVersion.type << "."
		<< spinnakerLibraryVersion.build << endl << endl;

	unsigned int numCameras = camList.GetSize();

	cout << "Number of cameras detected: " << numCameras << endl << endl;

	// Finish if there are no cameras
	if (numCameras == 0)
	{
		// Clear camera list before releasing system
		camList.Clear();

		// Release system
		system->ReleaseInstance();

		cout << "Not enough cameras!" << endl;

		return;
	}
	else {
		isCameraActive = true;
		pCam = camList.GetByIndex(0);
	}

	try
	{
		// Retrieve TL device nodemap and print device information
		INodeMap & nodeMapTLDevice = pCam->GetTLDeviceNodeMap();

		result = PrintDeviceInfo(nodeMapTLDevice);

		// Initialize camera
		pCam->Init();

		// Acquisition mode
		pCam->AcquisitionMode.SetValue(AcquisitionMode_Continuous);

		// set roi
		pCam->Width.SetValue(width);
		pCam->Height.SetValue(height);
		pCam->OffsetX.SetValue(_offset_x);
		pCam->OffsetY.SetValue(_offset_y);

		// exposure
		pCam->ExposureAuto.SetValue(ExposureAuto_Off);	// exposure set manually
		pCam->ExposureTime.SetValue(5000);	// micro sec

		// gain
		pCam->GainAuto.SetValue(GainAuto_Off);	// for over 200fps

	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
	}

	pCam->BeginAcquisition();

	srcImage.allocate(width, height, OF_IMAGE_GRAYSCALE);

	imageBuffer = new unsigned char[width * height];
}

//--------------------------------------------------------------
void ofxSpinnaker::update()
{
	if (!isCameraActive) return;

	try
	{
		grabNewImage();
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
	}

}

//--------------------------------------------------------------
void ofxSpinnaker::exit()
{
	//pCam->EndAcquisition();
	//cout << "end acquisition" << endl;

	//// Release system
	//system->ReleaseInstance();
}

//--------------------------------------------------------------
void ofxSpinnaker::draw(int x, int y)
{
	// Attempt to lock the mutex.  If blocking is turned on,
	if (lock())
	{
		srcImage.setFromPixels(imageBuffer, width, height, OF_IMAGE_GRAYSCALE);
		srcImage.update();
		srcImage.draw(x, y);
		unlock();
	}
	//ofSleepMillis(1);
}

//--------------------------------------------------------------
// This function prints the device information of the camera from the transport
// layer; please see NodeMapInfo example for more in-depth comments on printing
// device information from the nodemap.
int ofxSpinnaker::PrintDeviceInfo(INodeMap & nodeMap)
{
	int result = 0;

	cout << endl << "*** DEVICE INFORMATION ***" << endl << endl;

	try
	{
		FeatureList_t features;
		CCategoryPtr category = nodeMap.GetNode("DeviceInformation");
		if (IsAvailable(category) && IsReadable(category))
		{
			category->GetFeatures(features);

			FeatureList_t::const_iterator it;
			for (it = features.begin(); it != features.end(); ++it)
			{
				CNodePtr pfeatureNode = *it;
				cout << pfeatureNode->GetName() << " : ";
				CValuePtr pValue = (CValuePtr)pfeatureNode;
				cout << (IsReadable(pValue) ? pValue->ToString() : "Node not readable");
				cout << endl;
			}
		}
		else
		{
			cout << "Device control information not available." << endl;
		}
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}

//--------------------------------------------------------------
void ofxSpinnaker::grabNewImage()
{
	try
	{
		ImagePtr pResultImage = pCam->GetNextImage();

		if (pResultImage->IsIncomplete())
		{
			// Retreive and print the image status description
			cout << "Image incomplete: "
				<< Image::GetImageStatusDescription(pResultImage->GetImageStatus())
				<< "..." << endl << endl;
		}
		else
		{
			// Convert image to mono 8
			ImagePtr convertedImage = pResultImage->Convert(PixelFormat_Mono8, HQ_LINEAR);

			// Copy the input image data to the buffer for ofImage.
			memcpy(imageBuffer, (unsigned char*)convertedImage.get()->GetData(), width * height);

		}
		pResultImage->Release();

		//cout << endl;
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
	}

}

//--------------------------------------------------------------
void ofxSpinnaker::setFrameRate(float fps)
{
	pCam->AcquisitionFrameRate.SetValue(fps);
}

//--------------------------------------------------------------
void ofxSpinnaker::setExposure(float exposure_ms)
{
	pCam->ExposureTime.SetValue(exposure_ms);
}