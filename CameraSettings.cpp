#include "stdafx.h"
#include "CameraSettings.h"

#define APP_CAMERA _T("Camera")

using namespace std;
using namespace cv;

#define FPN_FRAME_COUNT 16

CCameraSettings::CCameraSettings(void)
{
	_usbCameraNum = 0;
	_simulateCamera = true;
	_bayerType = 0;

	_sensorSize = Size(640, 480);

	_nGain = 124;
	_nExposure = 31;
	_CameraChip = OV426;
}


CCameraSettings::~CCameraSettings(void)
{
}

void CCameraSettings::Copy(CCameraSettings *pSrc)
{
	// create a json from the source 
	Json::Value root;
	pSrc->Save(root);

	// Load settings from the json
	this->Load(root);
}

void CCameraSettings::Save(Json::Value &root)
{
	Json::Value arrayInputPoints;

	root["Center"]["X"] = _imageCenter.x;
	root["Center"]["Y"] = _imageCenter.y;

	root["Size"]["X"] = _imageSize.width;
	root["Size"]["Y"] = _imageSize.height;

	root["Gain Red"] = _scaleRed;
	root["Gain Green"] = _scaleGreen;
	root["Gain Blue"] = _scaleBlue;

	root["Bayer Type"] = _bayerType;

	root["Sensor Height"] = _sensorSize.height;
	root["Sensor Width"] = _sensorSize.width;

	root["Gain"] = _nGain;
	root["Exposure"] = _nExposure;

	root["Auto Exposure"] = (int)(_bAutoExposureEnabled ? 1 : 0);
	root["Auto Gain"] = (int)(_bAutoGainEnabled ? 1 : 0);

	root["Simulate Camera"] = (int)(_simulateCamera ? 1 : 0);

	root["USB Camera Number"] = _usbCameraNum;

	root["FPN Frame Count"] = _fpnCalFrameCount;

	root["Camera Angle"] = _cameraAngle;

	for (int i = 0; i < _inputPoints.size(); i++)
	{
		Json::Value value;
		value["X"] = _inputPoints[i].x;
		value["Y"] = _inputPoints[i].y;
		arrayInputPoints.append(value);
	}
	Json::Value arrayOutputPoints;
	for (int i = 0; i < _outputPoints.size(); i++)
	{
		Json::Value value;
		value["X"] = _outputPoints[i].x;
		value["Y"] = _outputPoints[i].y;
		arrayOutputPoints.append(value);
	}
	root["Input Points"] = arrayInputPoints;
	root["Output Points"] = arrayOutputPoints;

	Json::Value arrayDeadPixels;
	for (int i = 0; i < _deadPixels.size(); i++)
	{
		Json::Value value;
		value["X"] = _deadPixels[i].x;
		value["Y"] = _deadPixels[i].y;
		arrayDeadPixels.append(value);
	}
	root["Dead Pixels"] = arrayDeadPixels;
}

void CCameraSettings::Load(const Json::Value &root)
{
	_imageCenter.x = root["Center"].get("X", 0).asFloat();
	_imageCenter.y = root["Center"].get("Y", 0).asFloat();

	_imageSize.width = root["Size"].get("X", 0).asFloat();
	_imageSize.height = root["Size"].get("Y", 0).asFloat();

	_bayerType = root.get("Bayer Type", 0).asInt();

	_sensorSize.height = root.get("Sensor Height", _imageSize.height).asInt();
	_sensorSize.width = root.get("Sensor Width", _imageSize.width).asInt();

	_simulateCamera = (root.get("Simulate Camera", 0).asInt() == 1);

	_usbCameraNum = root.get("USB Camera Number", 0).asInt();
	CameraChip chipType = (_usbCameraNum == 5 ? OV9734 : OV426);
	if (_usbCameraNum == 0 || _simulateCamera)
	{
		_simulateCamera = true;
		chipType = SIMULATED;
	}
	SetCameraChip(chipType);

	EnableAutoExposure(root.get("Auto Exposure", 1).asInt() == 1);
	EnableAutoGain(root.get("Auto Gain", 1).asInt() == 1);

	SetGain(root.get("Gain", 124).asInt());
	SetExposure(root.get("Exposure", 31).asInt());

	double red = root.get("Gain Red", 1).asFloat();
	double green = root.get("Gain Green", 1).asFloat();
	double blue = root.get("Gain Blue", 1).asFloat();
	SetColorGains(red, green, blue);

	_fpnCalFrameCount = root.get("FPN Frame Count", FPN_FRAME_COUNT).asInt();

	_cameraAngle = root.get("Camera Angle", 0).asFloat();

	const Json::Value& arrayInputPoints = root["Input Points"];
	_inputPoints.clear();
	for (Json::ValueConstIterator itr = arrayInputPoints.begin(); itr != arrayInputPoints.end(); itr++) {
		Point2f pt;

		pt.x = itr->get("X", 0).asFloat();
		pt.y = itr->get("Y", 0).asFloat();
		_inputPoints.push_back(pt);
	}
	Json::Value arrayOutputPoints = root["Output Points"];
	_outputPoints.clear();
	for (Json::ValueConstIterator itr = arrayOutputPoints.begin(); itr != arrayOutputPoints.end(); itr++) {
		Point2f pt;

		pt.x = itr->get("X", 0).asFloat();
		pt.y = itr->get("Y", 0).asFloat();
		_outputPoints.push_back(pt);
	}

	Json::Value arrayDeadPixels = root["Dead Pixels"];
	_deadPixels.clear();
	for (Json::ValueConstIterator itr = arrayDeadPixels.begin(); itr != arrayDeadPixels.end(); itr++) {
		Point pt;

		pt.x = itr->get("X", 0).asInt();
		pt.y = itr->get("Y", 0).asInt();
		_deadPixels.push_back(pt);
	}

#ifdef _DEBUG
	_simulateCamera = true;
#endif
}

void CCameraSettings::UpdateInputPoint(int index, Point2f pt)
{
	if ((index >= 0) && (index < _inputPoints.size()))
	{
		_inputPoints[index] = pt;
	}
}

void CCameraSettings::UpdateOutputPoint(int index, Point2f pt)
{
	if ((index >= 0) && (index < _outputPoints.size()))
	{
		_outputPoints[index] = pt;
	}
}
