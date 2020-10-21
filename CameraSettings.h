#pragma once

#include <string>
#include "opencv2\core\core.hpp"
#include "json\json.h"

enum CameraChip
{
	SIMULATED,
	OV426,
	OV9734
};

class CCameraSettings
{
public:
	CCameraSettings(void);
	~CCameraSettings(void);

	void Copy(CCameraSettings *pSrc);

	void Save(Json::Value &root);
	void Load(const Json::Value &root);

	std::vector<cv::Point2f> GetInputPoints() { return _inputPoints; }
	std::vector<cv::Point2f> GetOutputPoints() { return _outputPoints; }
	std::vector<cv::Point> GetDeadPixels() { return _deadPixels; }

	void UpdateInputPoint(int index, cv::Point2f pt);
	void UpdateOutputPoint(int index, cv::Point2f pt);

	cv::Size GetSensorSize()	{ return _sensorSize; }
	cv::Size2f GetImageSize()			{ return _imageSize; }
	cv::Point2f GetImageCenter()	{ return _imageCenter; }
	void SetImageCenter(cv::Point2f center)	{ _imageCenter = center; }

	int GetFPNCalFrameCount()	{ return _fpnCalFrameCount; }

	int GetUSBCameraNum()	{ return _usbCameraNum; }
	bool IsCameraSimulated()	{ return _simulateCamera; }

	void SetCameraChip(CameraChip chip) { _CameraChip = chip; }
	CameraChip GetCameraChip()			{ return _CameraChip; }

	void SetExposure(int val)	{ _nExposure = val; }
	int GetExposure()			{ return _nExposure; }

	void SetGain(int val)		{ _nGain = val;  }
	int GetGain()				{ return _nGain; }

	void EnableAutoExposure(bool enable)	{ _bAutoExposureEnabled = enable; }
	bool IsAutoExposureEnabled()		{ return _bAutoExposureEnabled; }

	void EnableAutoGain(bool enable)	{ _bAutoGainEnabled = enable; }
	bool IsAutoGainEnabled()		{ return _bAutoGainEnabled; }

	void SetBayerType(int type)		{ _bayerType = type; }
	int GetBayerType()				{ return _bayerType; }

	void SetColorGains(float red, float green, float blue)
	{
		_scaleRed = red;
		_scaleGreen = green;
		_scaleBlue = blue;

	}
	void GetColorGains(float &red, float &green, float &blue)
	{
		red = _scaleRed;
		green = _scaleGreen;
		blue = _scaleBlue;
	}

	double GetCameraAngle()  { return _cameraAngle; }
	void SetCameraAngle(double angle)  { _cameraAngle = angle; }

private:
	std::vector<cv::Point2f> _inputPoints;
	std::vector<cv::Point2f> _outputPoints;
	std::vector<cv::Point> _deadPixels;
	cv::Point2f _imageCenter;
	cv::Size2f _imageSize;
	cv::Point2f _logPolarCenter;
	cv::Size _sensorSize;
	int _usbCameraNum;
	bool _simulateCamera;
	int _nGain;
	int _nExposure;
	bool _bAutoGainEnabled;
	bool _bAutoExposureEnabled;
	CameraChip _CameraChip;
	float _scaleRed;
	float _scaleBlue;
	float _scaleGreen;
	int _bayerType;
	int _fpnCalFrameCount;
	double _cameraAngle;
};

