#include "StdAfx.h"
#include "CameraData.h"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "CThinPlateSpline.h"
#include "LaplacianBlending.h"
#include "Thinning.h"
#include "TimingData.h"
#include <fstream>
#include <math.h>
#include "json\json.h"

using namespace std;
using namespace cv;

log4cxx::LoggerPtr CameraData::logger = log4cxx::Logger::getLogger("CameraData");

#define AUTO_WHITE_BRIGHTNESS 128
#define DYNAMIC_SIDE_SCALE_UPDATE 5
#define GAIN_CHANGE_IGNORE_FRAME_COUNT 5
#define MAX_SIDE_SCALE 5

//vector<cv::Point> iP, iiP
double scaleImage = 4.0;
Point imageOrigin;
string imageName;
int indexPoint;
//Mat imgLarge, imgMaskLarge, imgMosaic, imgMosaicCount, imgMaskMosaic;
Mat imgMosaicInput, imgMaskInput;


CameraData::CameraData(CameraType type, MosaicData *pMosaicData, CEnhancementControl *pEnhancement, ProcTimes *pProcTimes):
_laplacianBlending(pProcTimes, 1)
{
	_cameraType = type;
	_cameraState = eConstructed;
	_pMosaicData = pMosaicData;
	_pEnhancementControl = pEnhancement;
	_pProcTimes = pProcTimes;
	_pMicroController = NULL;

	_bExiting = false;
	_hThread = NULL;

	_averageRed = 0;
	_averageBlue = 0;
	_averageGreen = 0;

	_saturationRed = 0;
	_saturationBlue = 0;
	_saturationGreen = 0;

	_bFPNCalActive = false;
	_bAbortFPNCal = false;
	_nFPNAvgCount = -1;

	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	_perfFreq = li.QuadPart;
	_lastFrameTime = 0;
	_frameRate = 0;
	_frameCount = 0;
	_eImageStitchingMode = eNormal;
	_bSimType = eNone;
	_temporalAverageQueueSize = 0;
	_ignoreFrameCount = 0;
	_gainAutoCurrentIndex = 0;

	_eMeasurementType = eNoMeasurement;
	_eMeasurementTypeNew = eNoMeasurement;
	_dMeasurementValue = 0;

	if (_cameraType == eFront)
	{
		_gainAutoValues.push_back(0);
		_gainAutoValues.push_back(20);
		_gainAutoValues.push_back(40);
		_gainAutoValues.push_back(60);

		_upsampleFactor = 1;
	}
	else
	{
		_gainAutoValues.push_back(0);
		_gainAutoValues.push_back(10);
		_gainAutoValues.push_back(25);
		_gainAutoValues.push_back(50);
		_gainAutoValues.push_back(150);
		_gainAutoValues.push_back(250);
		_gainAutoValues.push_back(500);
		_gainAutoValues.push_back(1000);

		_upsampleFactor = _pEnhancementControl->GetCurrentSettings()->GetUpsampleFactor();
	}
	
	UpdateLookup(1.0f, 1.0f);

#ifndef STITCH_TEST
	_hCaptureEvent = CreateEvent(
		NULL,   // default security attributes
		FALSE,  // auto-reset event object
		FALSE,  // initial state is nonsignaled
		NULL);  // unnamed object

	_hWarpEvent = CreateEvent(
		NULL,   // default security attributes
		FALSE,  // auto-reset event object
		FALSE,  // initial state is nonsignaled
		NULL);  // unnamed object

	_hUpdateEvent = CreateEvent(
		NULL,   // default security attributes
		FALSE,  // auto-reset event object
		FALSE,  // initial state is nonsignaled
		NULL);  // unnamed object

	StartWarpThread();
#endif
}

CameraData::~CameraData(void)
{
	LOG4CXX_INFO(logger, "Destroying " << CameraNames[(int)_cameraType]);

	StopWarpThread();
}

void CameraData::StartWarpThread()
{
	if (_hThread == NULL)
	{
		// Create a thread
		DWORD dwThreadID;
		_hThread = CreateThread(
			NULL,         // default security attributes
			0,            // default stack size
			(LPTHREAD_START_ROUTINE)&CameraData::ThreadProc,
			(LPVOID)this,         // no thread function arguments
			0,            // default creation flags
			&dwThreadID); // receive thread identifier
	}
}

void CameraData::StopWarpThread()
{
	try
	{

		LOG4CXX_INFO(logger, "StopWarpThread for " << CameraNames[(int)_cameraType] << ": Clearing image");

		ClearImage();

		LOG4CXX_INFO(logger, "StopWarpThread for " << CameraNames[(int)_cameraType] << ": Waiting for warp thread to finish");

		_bExiting = true;
		if (_hThread != NULL)
		{
			WaitForSingleObject(_hThread, INFINITE);
			_hThread = NULL;
		}
		_bExiting = false;

		LOG4CXX_INFO(logger, "StopWarpThread for " << CameraNames[(int)_cameraType] << ": Setting mosic portion to black");

		_imgMosaic.setTo(0);
		_pMosaicData->SetCameraMosaic(_cameraType, _imgMosaic, _maskMosaic, _rectImageWarp, _rectImageBorder);

		LOG4CXX_INFO(logger, "StopWarpThread for " << CameraNames[(int)_cameraType] << ": Complete");
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger, "StopWarpThread exception occured for camera " << CameraNames[(int)_cameraType]);
	}
}

string CameraData::GetName()
{
	stringstream ss;
	ss << "CAM" << _cameraSettings.GetUSBCameraNum();
	return ss.str();
}

void CameraData::MicroControllerConnected(CMicroController *pMicroController)
{
	LOG4CXX_INFO(logger, "Redownloading settings after connection to MicroController for camera " << CameraNames[(int)_cameraType]);

	_pMicroController = pMicroController;

	_cameraState = eConstructed;
	StartCamera(true);
}

void CameraData::StartCamera(bool force)
{
	//if (_cameraState == eNotPresent)
	//{
	//	LOG4CXX_INFO(logger, "Ignoring StartCamera, because previous communication failed for camera " << CameraNames[(int)_cameraType]);
	//	return;
	//}

	//if (!force && (_cameraState == eInit))
	//{
	//	LOG4CXX_INFO(logger, "Attempting to start camera " << CameraNames[(int)_cameraType] << ", already in Init state.  Setting to eNotPresent state");
	//	_cameraState = eNotPresent;
	//	return;
	//}
	LOG4CXX_INFO(logger, "Starting camera " << CameraNames[(int)_cameraType]);

	if (force)
	{
		SetCameraChip(_cameraSettings.GetCameraChip());
		int retVal = InitCameraChip(false);
		if (retVal == 0)
		{
			RefreshSensorSettings();
		}
		else
		{
			_cameraState = eNotPresent;
		}

		_cameraState = eInit;
	}
	else
	{
		_cameraState = eBlank;
		SetEvent(_hWarpEvent);
	}
}

void CameraData::ClearImage()
{
	// Clear the image
	Size sensorSize = _cameraSettings.GetSensorSize();
	Mat img16 = Mat::zeros(sensorSize, CV_16U);
	SetBayerImage(img16, false, "");
}


int CameraData::InitCameraChip(bool force)
{
	int retVal = -1;
	if (_pMicroController != NULL)
	{
		retVal = _pMicroController->SendCameraInit(_cameraSettings.GetUSBCameraNum(), force);
	}

	return retVal;
}

void CameraData::SetCameraChip(CameraChip chip)
{
	_cameraSettings.SetCameraChip(chip);

	// Update the HW
	if (_pMicroController != NULL)
	{
		_pMicroController->SetCameraChipType(_cameraSettings.GetUSBCameraNum(), chip);
	}
}

void CameraData::SetExposure(int val)
{
	_cameraSettings.SetExposure(val);

	// Update the HW
	if (_pMicroController != NULL)
	{
		_pMicroController->SetShutterValue(_cameraSettings.GetUSBCameraNum(), val);
	}
}

void CameraData::SetGain(int val)
{
	int oldHighByte = ((_cameraSettings.GetGain() >> 8) & 0xFF);
	int newHighByte = ((val >> 8) & 0xFF);
	if (oldHighByte != newHighByte)
	{
		_ignoreFrameCount = max(_ignoreFrameCount, 5);
		LOG4CXX_DEBUG(logger, "Ignoring " << _ignoreFrameCount << " with gain change");
	}

	// Find nearest gain index for FPN
	int closestIndex = FindClosestGainIndex(val);
	_gainAutoCurrentIndex = closestIndex;

	_cameraSettings.SetGain(val);

	// Update the HW
	if (_pMicroController != NULL)
	{
		_pMicroController->SetGainValue(_cameraSettings.GetUSBCameraNum(), val);
	}
}

void CameraData::SetColorGains(float red, float green, float blue)
{
	_cameraSettings.SetColorGains(red, green, blue);

	// Update the HW
	if (_pMicroController != NULL)
	{
		_pMicroController->SetWhiteBalanceEnable(_cameraSettings.GetUSBCameraNum(), true);
		_pMicroController->SetWhiteBalanceGains(_cameraSettings.GetUSBCameraNum(), red, green, blue);
	}
}

void CameraData::EnableAutoExposure(bool val)
{
	_cameraSettings.EnableAutoExposure(val);

	// Update the HW
	if (_pMicroController != NULL)
	{
		_pMicroController->SetAutoShutterGain(_cameraSettings.GetUSBCameraNum(), 
			_cameraSettings.IsAutoExposureEnabled(),
			_cameraSettings.IsAutoGainEnabled());
	}
}

void CameraData::EnableAutoGain(bool val)
{
	_cameraSettings.EnableAutoGain(val);

	// Update the HW
	if (_pMicroController != NULL)
	{
		_pMicroController->SetAutoShutterGain(_cameraSettings.GetUSBCameraNum(),
			_cameraSettings.IsAutoExposureEnabled(),
			_cameraSettings.IsAutoGainEnabled());
	}
}

void CameraData::ResetTempImages()
{
	_imgTempImage = Mat();
	_imgTempMask = Mat();
	_maskImageWarp = Mat();
	
	_imgMosaic = Mat();
	_maskMosaic = Mat();
}


void CameraData::UpdateThinPlateSpline()
{
	// Create the point lists with the image locations
	double mosaicDim = min(_pMosaicData->GetSize().width / 2, _pMosaicData->GetSize().height / 2);
	Size2f imageSize = _cameraSettings.GetImageSize();
	imageSize.width *= _upsampleFactor;
	imageSize.height *= _upsampleFactor;
	vector<Point2f> inputPoints = _cameraSettings.GetInputPoints();
	vector<Point2f> outputPoints = _cameraSettings.GetOutputPoints();
	vector<Point> iP, iiP;
	for (int i = 0; i < inputPoints.size(); i++)
	{
		iP.push_back(Point((inputPoints[i].x + 1.0) *imageSize.width / 2,
			(inputPoints[i].y + 1.0) * imageSize.width / 2));
		iiP.push_back(Point(outputPoints[i].x * mosaicDim + _pMosaicData->GetCenter().x,
			outputPoints[i].y * mosaicDim + _pMosaicData->GetCenter().y));
	}

	_tps.setCorrespondences(iP, iiP);
	_tps.setSourceSize(imageSize);
}

void CameraData::Load(string folder)
{
	_criticalSectionMosaic.Lock();

	stringstream ssName;
	ssName << folder << "\\" << CameraNames[(int)_cameraType] << ".json";

	LOG4CXX_INFO(logger, "Camera " << CameraNames[(int)_cameraType] << ": Loading from " << ssName.str());

	ifstream file(ssName.str());

	Json::Value root;
	Json::Reader reader;

	bool parsingSuccessful = reader.parse(file, root);
	if (parsingSuccessful)
	{
		_cameraSettings.Load(root);
	}
	else
	{
		LOG4CXX_ERROR(logger, "Camera " << CameraNames[(int)_cameraType] << " doesn't have a config file");
	}

	file.close();

	Size sensorSize = _cameraSettings.GetSensorSize();
	_imgSensorRGB = Mat::zeros(sensorSize, CV_8UC3);
	_imgSensorRGB2 = Mat::zeros(sensorSize, CV_8UC3);


	_imgFPNCal = vector<Mat>(_gainAutoValues.size());
	for (int i = 0; i < _gainAutoValues.size(); i++)
	{
		ssName = stringstream();
		ssName << folder << "\\" << CameraNames[(int)_cameraType] << "FPN_Gain" << _gainAutoValues[i] << ".raw";
		FILE *fileFPN = fopen(ssName.str().c_str(), "rb");

		Size sizeFPN = sensorSize;
		if (_imgFPNCal[i].size() != sizeFPN)
		{
			_imgFPNCal[i] = Mat::zeros(sizeFPN, CV_16U);
		}

		if (fileFPN)
		{
			fread(_imgFPNCal[i].data, 2, sensorSize.width * sensorSize.height, fileFPN);
			fclose(fileFPN);
		}
	}

	// Load the front scale lookup table
	if (_cameraType == eFront)
	{
		_scaleLookup = vector<float>();

		ssName = stringstream();
		ssName << folder << "\\" << CameraNames[(int)_cameraType] << "ScaleLookup.txt";
		ifstream fileLookup(ssName.str().c_str());
		if (fileLookup.is_open())
		{ 
			while (!fileLookup.eof())
			{
				float val;
				fileLookup >> val;
				_scaleLookup.push_back(val);
			}
			fileLookup.close();
		}
	}
	
	_criticalSectionMosaic.Unlock();

	// Update the camera operation with the new settings
	RefreshCameraSettings();
}

void CameraData::RefreshCameraSettings()
{
	_criticalSectionMosaic.Lock();

	_imgMaskFromMosaic = Mat();

	if (_bSimType == eNone)
	{
		Size sensorSize = _cameraSettings.GetSensorSize();
		Size upSampleSize = Size(sensorSize.width * _upsampleFactor, sensorSize.height * _upsampleFactor);
		_imgRaw = Mat::zeros(upSampleSize, CV_8UC3);
	}

	ResetTempImages();
	UpdateThinPlateSpline();

	_criticalSectionMosaic.Unlock();
}

void CameraData::RefreshSensorSettings()
{
	float r, g, b;
	_cameraSettings.GetColorGains(r, g, b);
	SetColorGains(r, g, b);
	EnableAutoExposure(_cameraSettings.IsAutoExposureEnabled());
	EnableAutoGain(_cameraSettings.IsAutoGainEnabled());
	SetExposure(_cameraSettings.GetExposure());
	SetGain(_cameraSettings.GetGain());
	ResetImageOutOfRange();
}

void CameraData::ResetImageOutOfRange()
{
	_imageOutOfRange = false;
	_imageOutOfRangeIgnoreCount = 0;
}

void CameraData::Save(string folder)
{
	stringstream ssName;
	ssName << folder << "\\" << CameraNames[(int)_cameraType] << ".json";

	LOG4CXX_INFO(logger, "Camera " << CameraNames[(int)_cameraType] << ": Saving to " << ssName.str());

	Json::Value root;
	
	_cameraSettings.Save(root);

	ofstream file(ssName.str());
	Json::StreamWriterBuilder styledWriterBuilder;
	styledWriterBuilder.settings_["precision"] = 5;
	file << Json::writeString(styledWriterBuilder, root);
	file.close();

	for (int i = 0; i < _gainAutoValues.size(); i++)
	{
		ssName = stringstream();
		ssName << folder << "\\" << CameraNames[(int)_cameraType] << "FPN_Gain" << _gainAutoValues[i] << ".raw";
		FILE *fileFPN = fopen(ssName.str().c_str(), "wb");
		if (fileFPN)
		{
			fwrite(_imgFPNCal[i].data, 2, _imgFPNCal[i].rows * _imgFPNCal[i].cols, fileFPN);
			fclose(fileFPN);
		}
	}

	if (_cameraType == eFront)
	{
		ssName = stringstream();
		ssName << folder << "\\" << CameraNames[(int)_cameraType] << "ScaleLookup.txt";
		ofstream fileLookup(ssName.str().c_str());
		if (fileLookup.is_open())
		{
			for (int i = 0; i < _scaleLookup.size(); i++)
			{
				fileLookup << _scaleLookup[i] << std::endl;
			}
			fileLookup.close();
		}
	}
}

double CameraData::GetAngle()
{
	double angle = 0.0;
	switch (_cameraType)
	{

	case eTop:
		angle = 90.0;
		break;
	case  eLeft:
		angle = 180.0;
		break;
	case  eBottom:
		angle = 270.0;
		break;
	case eRight:
		angle = 0.0;
		break;
	}

	return angle;
}

void CameraData::GetMosaicMask(Size mosaicSize, Mat &mask)
{
	int mosaicDim = min(mosaicSize.height, mosaicSize.width);
	Point mosaicCenter = Point(mosaicSize.width / 2, mosaicSize.height / 2);
	mask.setTo(0);
	if (_cameraType == eFront)
	{
		//circle(mask, mosaicCenter, mosaicDim*_stichRadius / 2, Scalar(255, 255, 255), -1);
		mask.setTo(255);
	}
	else
	{
		double border = 0;// _pMosaicData->IsBlendingEnabled() ? 5 : 0;
		double angle = GetAngle();

		// Create the corners of the mask
		Point corner_points[1][3];
		double angle1Rad = (angle + 45.0 + border)*PI / 180.0;
		double angle2Rad = (angle - 45.0 - border)*PI / 180.0;
		double cosAngle1 = cos(angle1Rad);
		double cosAngle2 = cos(angle2Rad);
		double sinAngle1 = sin(angle1Rad);
		double sinAngle2 = sin(angle2Rad);
		corner_points[0][0] = mosaicCenter;
		corner_points[0][1] = mosaicCenter + Point(mosaicDim * cosAngle1, -mosaicDim * sinAngle1);
		corner_points[0][2] = mosaicCenter + Point(mosaicDim * cosAngle2, -mosaicDim * sinAngle2);
		const Point* ppt[1] = { corner_points[0] };
		int npt[] = { 3 };

		fillPoly(mask,
			ppt,
			npt,
			1,
			Scalar(255, 255, 255));
	}
}

void CameraData::GetCameraMask(Size mosaicSize, Mat &mask)
{
	Size2f imageSize = _cameraSettings.GetImageSize();
	imageSize.width *= _upsampleFactor;
	imageSize.height *= _upsampleFactor;
	Point2f origin = Point2f(0, 0);
	Rect roi = Rect(origin, imageSize);
	
	if (0)//_cameraType != eFront)
	{
		// Crop out the central circle of the front camera
		mask(roi).setTo(0);
		float radius = 1.05 * roi.width / 2;
		circle(mask, Point(roi.width / 2, roi.height / 2), (int) round(radius),
			Scalar(255, 255, 255), -1);
	}
	else
	{
		mask(roi).setTo(255);
		// Crop out the corners of the front camera
		double cornerPercent = 0.2; // javqui  original 0.2;
		if (_cameraType == eFront)
			cornerPercent = 0.2;

		double cornerHeight = cornerPercent * imageSize.height;
		double cornerWidth = cornerPercent * imageSize.width;
		Point corner_points[4][3];
		// Top Left
		corner_points[0][0] = roi.tl();
		corner_points[0][1] = corner_points[0][0] + Point(cornerWidth, 0);
		corner_points[0][2] = corner_points[0][0] + Point(0, cornerHeight);
		// Top Right
		corner_points[1][0] = Point(roi.br().x, roi.tl().y);
		corner_points[1][1] = corner_points[1][0] + Point(-cornerWidth, 0);
		corner_points[1][2] = corner_points[1][0] + Point(0, cornerHeight);
		// Bottom Left
		corner_points[2][0] = Point(roi.tl().x, roi.br().y);
		corner_points[2][1] = corner_points[2][0] + Point(cornerWidth, 0);
		corner_points[2][2] = corner_points[2][0] + Point(0, -cornerHeight);
		// Bottom Right
		corner_points[3][0] = roi.br();
		corner_points[3][1] = corner_points[3][0] + Point(-cornerWidth, 0);
		corner_points[3][2] = corner_points[3][0] + Point(0, -cornerHeight);
		
		const Point* ppt[4] = { corner_points[0], corner_points[1], corner_points[2], corner_points[3] };
		int npt[] = { 3, 3, 3, 3 };

		fillPoly(mask,
			ppt,
			npt,
			4,
			Scalar(0, 0, 0));
	}
}

double CameraData::CalculateSaturation(Mat& image, Mat& mask)
{
	int valThresh = 200; // for some reason the side cameras don't give full signal when saturated
	if (_cameraType == eFront)
		valThresh = 225;

	Mat imgThreshold;

	threshold(image, imgThreshold, valThresh, 255, THRESH_BINARY);
	imgThreshold &= mask;
	Scalar satCount = sum(imgThreshold);
	Scalar totalCount = sum(mask);
	double sat = satCount[0];
	double total = totalCount[0];
	return sat / total;
}

void CameraData::DebayerImage()
{
	if (_bSimType != eRaw)
	{
		int bayerOp = _cameraSettings.GetBayerType() + CV_BayerBG2BGR;
		
		if (_cameraType == eFront)
		{
			cv::cvtColor(_imgBayer, _imgSensorRGB, bayerOp);
			cv::resize(_imgSensorRGB, _imgRaw, _imgRaw.size());
		}
		else
		{
			cv::cvtColor(_imgBayer, _imgSensorRGB, bayerOp);
			
			CEnhancementSettings *pSettings = _pEnhancementControl->GetCurrentSettings();
			if (pSettings->IsSpatialAveragingEnabled()
				&& (pSettings->GetSpatialAveraging() > 0))
			{
				flip(_imgSensorRGB, _imgSensorRGB, 1);

				int kernel = 2 * pSettings->GetSpatialAveraging() + 1;
				if (pSettings->GetSpatialMedianAveraging())
				{
					medianBlur(_imgSensorRGB, _imgSensorRGB2, kernel);
				}
				else
				{
					blur(_imgSensorRGB, _imgSensorRGB2, Size(kernel, kernel));
				}
			}
			else
			{
				flip(_imgSensorRGB, _imgSensorRGB2, 1);
			}

			cv::resize(_imgSensorRGB2, _imgRaw, _imgRaw.size());
		}

		try
		{
			Rect cameraROI = GetCameraROI();
			Mat imgRoi = _imgRaw(cameraROI);
			DoMeasurement(imgRoi, _imgMaskFromMosaic);
		}
		catch (cv::Exception ex)
		{
			LOG4CXX_ERROR(logger, "Camera " + CameraNames[(int)_cameraType] + ": Exception in DoMeasurement" + ex.msg);
		}
		catch (...)
		{
			LOG4CXX_ERROR(logger, "Camera " + CameraNames[(int)_cameraType] + ": Exception in DoMeasurement");
		}
	}
}

void CameraData::UpdateFrontScaleImage(float slope)
{
	_fScaleSlope = slope;

	Point2f center = _cameraSettings.GetImageCenter();

	for (int row = 0; row < _imgScale.rows; row++)
	{
		for (int col = 0; col < _imgScale.cols; col++)
		{
			Point2f  point = Point2f(col, row);
			float scale = 1.0;
			if (_scaleLookup.size() > 1)
			{
				int dist = (int)round(_hypot(col - center.x, row - center.y));
				dist = min(dist, (int)_scaleLookup.size()-1);
				scale = (_scaleLookup[dist]-1)*slope + 1;
			}
			
			_imgScale.at<float>(point) = scale;
		}
	}
}

void CameraData::UpdateScaleImage(float slope, float offset)
{
	_fScaleSlope = slope;
	_fScaleOffset = offset;

	float angleOffset = 0;
	switch (_cameraType)
	{
	case eLeft: angleOffset = 180; break;
	case eTop: angleOffset = 270; break;
	case eRight: angleOffset = 0; break;
	case eBottom: angleOffset = 90; break;
	}

	double angle = _cameraSettings.GetCameraAngle() + angleOffset;
	Point2f center = _cameraSettings.GetImageCenter();
	double a = -cos(angle* PI / 180);
	double b = sin(angle* PI / 180);
	double c = -a * center.x - b * center.y;
	for (int row = 0; row < _imgScale.rows; row++)
	{
		for (int col = 0; col < _imgScale.cols; col++)
		{
			Point2f  point = Point2f(col, row);
			double dist = (a * point.x + b * point.y + c) / hypot(a, b);
			double scale = offset - (slope * dist / _imgScale.cols);
			scale = max(1.0, scale);
			scale = min(1.25, scale);
			_imgScale.at<float>(point) = scale;
		}
	}
}

void CameraData::UpdateDynamicScaleImage(Mat image, float offset)
{
	_fScaleOffset = offset;

	if (_imgScaleDynamic.size() != _rectImageWarp.size())
	{
		_imgScaleDynamic = Mat(_rectImageWarp.size(), CV_32FC3);
		_imgScaleDynamic.setTo(Vec3f(1.0f, 1.0f, 1.0f));
	}

	try
	{
		Rect frontRect = _pMosaicData->GetFrontBorderRect();
		if ((frontRect.height == 0) || (frontRect.width == 0))
			return;

		Scalar sideAverage = _pMosaicData->GetBorderAverage(_cameraType, image);
		Scalar frontAverage = _pMosaicData->GetFrontBorderAverageValue(_cameraType);
		if ((frontAverage[0] <= 0) && (frontAverage[1] <= 0) && (frontAverage[2] <= 0))
			return;

		int endPos, startPos;

		switch (_cameraType)
		{
		case eLeft:
			endPos = frontRect.x - _rectImageWarp.x;
			startPos = round(_pEnhancementControl->GetCurrentSettings()->GetSideScaleOffset() * endPos);
			break;

		case eTop:
			endPos = frontRect.y - _rectImageWarp.y;
			startPos = round(_pEnhancementControl->GetCurrentSettings()->GetSideScaleOffset() * endPos);
			break;

		case eRight:
			endPos = frontRect.br().x - _rectImageWarp.x;
			startPos = _rectImageWarp.width - (_pEnhancementControl->GetCurrentSettings()->GetSideScaleOffset() * (_rectImageWarp.width - endPos));
			break;

		case eBottom:
			endPos = frontRect.br().y - _rectImageWarp.y;
			startPos = _rectImageWarp.height - (_pEnhancementControl->GetCurrentSettings()->GetSideScaleOffset() * (_rectImageWarp.height - endPos));
			break;
		}


		Vec3f scaleRatio = Vec3f(frontAverage[0] / sideAverage[0],
			frontAverage[1] / sideAverage[1],
			frontAverage[2] / sideAverage[2]);

		// Limit the amount of scaling done on each color
		bool maxScaleExceeded = false;
		for (int i = 0; i < 3; i++)
		{
			if (scaleRatio[i] > MAX_SIDE_SCALE)
			{
				maxScaleExceeded = true;
				break;
			}
				
		}

		// If max scale exceeded, don't update scale image
		if (maxScaleExceeded)
		{
			return;
		}

		// Set the scale based on the the position with linear equation
		scaleRatio = (scaleRatio - Vec3f (1, 1, 1))/(endPos - startPos);
		for (int row = 0; row < _imgScaleDynamic.rows; row++)
		{
			for (int col = 0; col < _imgScaleDynamic.cols; col++)
			{
				Vec3f scales = Vec3f(1.0f, 1.0f, 1.0f);

				switch (_cameraType)
				{
				case eLeft:
					if (col > startPos)
					{
						scales += scaleRatio * (col - startPos);
					}
					break;
				case eTop:
					if (row > startPos)
					{
						scales += scaleRatio * (row - startPos);
					}
					break;
				case eRight:
					if (col < startPos)
					{
						scales += scaleRatio * (col - startPos);
					}
					break;
				case eBottom:
					if (row < startPos)
					{
						scales += scaleRatio * (row - startPos);
					}
					break;
				}

				float maxScale = 10.0f;
				scales[0] = min(scales[0], maxScale);
				scales[1] = min(scales[1], maxScale);
				scales[2] = min(scales[2], maxScale);

				_imgScaleDynamic.at<Vec3f>(Point(col, row)) = scales;
			}
		}
	}
	catch (cv::Exception ex)
	{
		LOG4CXX_ERROR(logger, "Camera " + CameraNames[(int)_cameraType] + ": Unable to update scale image:" + ex.msg);
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger, "Camera " + CameraNames[(int)_cameraType] + ": Unable to update scale image");
	}
}

void CameraData::UpdateLookup(float brightness, float gamma)
{
	float offset = 255 * brightness/4.0;

	for (int i = 0; i < MAX_RAW_VAL; i++)
	{
		_pGammaLookup[i] = saturate_cast<uchar>(offset + pow((float)i / (MAX_RAW_VAL - 1), gamma) * 255.0f);
	}

	_fGamma = gamma;
	_fBrightness = brightness;

	/*string fname = "C:\\NGE\\DebugImages\\" + CameraNames[(int)_cameraType] + "_Lookup.csv";
	ofstream file(fname);
	file << "Gamma = " << gamma << endl << endl;
	file << "Brightness = " << brightness << endl << endl;
	file << "Camera HDR,Ouput 8bit" << endl;
	for (int i = 0; i < MAX_RAW_VAL; i++)
	{
		file << i << "," << (int)_pGammaLookup[i] << endl;
	}*/
}

void CameraData::ApplyLookup(const Mat& src, Mat& dst)
{
	MatConstIterator_<ushort> itSrc = src.begin<ushort>();
	MatConstIterator_<ushort> endSrc = src.end<ushort>();
	MatIterator_<uchar> itDst = dst.begin<uchar>();

	for (; itSrc != endSrc; ++itSrc, ++itDst)
	{
		*itDst = _pGammaLookup[(*itSrc)];
	}
}


void CameraData::WhiteBalance(const Mat imgUnbalanced, Mat& imgLarge, Mat& mask)
{
	vector<Mat> layers;
	vector<double> means;
	vector<double> maxes;

	split(imgUnbalanced, layers);

	//layers[0] *= _scaleBlue;
	//layers[1] *= _scaleGreen;
	//layers[2] *= _scaleRed;

	// Store the average values
	_averageBlue = mean(layers[0], mask).val[0];
	_averageGreen = mean(layers[1], mask).val[0];
	_averageRed = mean(layers[2], mask).val[0];

	// If the image is too dark, mark for restart
	/* Disable image out of range check, since it is causing constant recovery in dark situations
	_imageOutOfRangeIgnoreCount++;
	if (_imageOutOfRangeIgnoreCount > 5)
	{
		_imageOutOfRangeIgnoreCount = -1;
	}
	if (!_imageOutOfRange && (_imageOutOfRangeIgnoreCount == -1))
	{
		double DARK_THRESH = 40;
		if ((_averageBlue < DARK_THRESH) && (_averageGreen < DARK_THRESH) && (_averageRed < DARK_THRESH))
		{
			LOG4CXX_ERROR(logger, CameraNames[(int)_cameraType] << " has all chanel avg less than " << DARK_THRESH << ". Setting Image Out Of Range flag");
			_imageOutOfRange = true;
		}
		double BRIGHT_THRESH = 225;
		if ((_averageBlue > BRIGHT_THRESH) && (_averageGreen > BRIGHT_THRESH) && (_averageRed > BRIGHT_THRESH))
		{
			LOG4CXX_ERROR(logger, CameraNames[(int)_cameraType] << " has all chanel avg more than " << BRIGHT_THRESH << ". Setting Image Out Of Range flag");
			_imageOutOfRange = true;
		}
	}*/

	// Find the amount of saturation
	_saturationBlue = CalculateSaturation(layers[0], mask);
	_saturationGreen = CalculateSaturation(layers[1], mask);
	_saturationRed = CalculateSaturation(layers[2], mask);

	// Put the layers back together
	merge(layers, imgLarge);
}

int CameraData::CalibrateWhiteBalance(double calRedComp, double calGreenComp, double calBlueComp)
{
	int ret = -1;

	try
	{
		// Calculate the average of each channel and set the gains accordingly
		Mat image = GetCameraImage();
		if ((image.size().height > 0) && (image.size().width > 0))
		{
			// Determine the average values in the image
			vector<Mat> layers;
			split(image, layers);
			double blueImageMean = mean(layers[0])[0];
			double greenImageMean = mean(layers[1])[0];
			double redImageMean = mean(layers[2])[0];

			// Determine the scale factor from the ideal cal values
			double blueIdealMean = greenImageMean * calBlueComp;
			double greenIdealMean = greenImageMean * calGreenComp;
			double redIdealMean = greenImageMean * calRedComp;

			double blueIdealScale = blueImageMean / blueIdealMean;
			double greenIdeaScale = greenImageMean / greenIdealMean;
			double redIdealScale = redImageMean / redIdealMean;

			// Determine the original values without the camera gain applied
			float scaleRed, scaleGreen, scaleBlue;
			_cameraSettings.GetColorGains(scaleRed, scaleGreen, scaleBlue);
			//blueImageMean /= scaleBlue;
			//greenImageMean /= scaleGreen;
			//redImageMean /= scaleRed;

			//double blueScaleNew = AUTO_WHITE_BRIGHTNESS / blueMean[0];
			//double greenScaleNew = AUTO_WHITE_BRIGHTNESS / greenMean[0];
			//double redScaleNew = AUTO_WHITE_BRIGHTNESS / redMean[0];
			double blueScaleNew = scaleBlue / blueIdealScale;
			double greenScaleNew = scaleGreen / greenIdeaScale;
			double redScaleNew = scaleRed / redIdealScale;

			LOG4CXX_INFO(logger, "CalibrateWhiteBalance Red Scale " << scaleRed << " -> " << redScaleNew);
			LOG4CXX_INFO(logger, "CalibrateWhiteBalance Green Scale " << scaleGreen << " -> " << greenScaleNew);
			LOG4CXX_INFO(logger, "CalibrateWhiteBalance Blue Scale " << scaleBlue << " -> " << blueScaleNew);

			//LOG4CXX_INFO(logger, printf("CalibrateWhiteBalance Red Scale %.6f -> %.6f",scaleRed,redScaleNew));
			//LOG4CXX_INFO(logger, printf("CalibrateWhiteBalance Green Scale %.6f -> %.6f",scaleGreen,greenScaleNew));
			//LOG4CXX_INFO(logger, printf("CalibrateWhiteBalance Blue Scale %.6f -> %.6f",scaleBlue,blueScaleNew));

			SetColorGains(redScaleNew, greenScaleNew, blueScaleNew);

			ret = 0;
		}
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger, "Error calibrating white balance ");
	}

	return ret;
}

void CameraData::CalibrateFPN()
{
	LOG4CXX_INFO(logger, "Starting FPN Calibration for " << CameraNames[(int)_cameraType]);

	_nExposureBeforeFPNCal = _cameraSettings.GetExposure();
	_bAutoExposureBeforeFPNCal = _cameraSettings.IsAutoExposureEnabled();
	_nGainBeforeFPNCal = _cameraSettings.GetGain();
	_bAutoGainBeforeFPNCal = _cameraSettings.IsAutoGainEnabled();
	_imgFPNCalBeforeFPNCal = GetFPNCalData();
	
	EnableAutoGain(false);
	EnableAutoExposure(false);
	SetExposure(0);
	_gainAutoCurrentIndex = 0;
	_imgBayer.setTo(0);

	_bAbortFPNCal = false;
	_bFPNCalActive = false; // Will be set true in NextFPNGain();

	NextFPNGain();
}

void CameraData::NextFPNGain()
{
	LOG4CXX_INFO(logger, "Next FPN gain index for " << CameraNames[(int)_cameraType] << " to " << _gainAutoValues[_gainAutoCurrentIndex] << "[" << _gainAutoCurrentIndex << "]");

	_imgFPNAvg = Mat::zeros(_imgFPNCal[_gainAutoCurrentIndex].size(), CV_32F);
	SetGain(_gainAutoValues[_gainAutoCurrentIndex]);

	_ignoreFrameCount = 15; // wait 0.5 sec for settings to take effect

	_bFPNCalActive = true;
	_nFPNAvgCount = 0; // Start FPN
}

void CameraData::CompleteFPNCal()
{
	EnableAutoExposure(_bAutoExposureBeforeFPNCal);
	SetExposure(_nExposureBeforeFPNCal);

	EnableAutoGain(_bAutoGainBeforeFPNCal);
	SetGain(_nGainBeforeFPNCal);

	_bFPNCalActive = false;
}


void CameraData::AbortFPNCal()
{
	LOG4CXX_INFO(logger, "Aborting FPN Calibration for " << CameraNames[(int)_cameraType]);
	
	_bAbortFPNCal = true;
	
	CompleteFPNCal();

	// Restore the old FPN calibration
	SetFPNCalData(_imgFPNCalBeforeFPNCal);
}

void CameraData::GetLastChannelValues(double &red, double &green, double &blue)
{
	//_criticalSection.Lock();

	red = _averageRed;
	green = _averageGreen;
	blue = _averageBlue;

	//_criticalSection.Unlock();
}

void CameraData::GetLastSaturationValues(double &red, double &green, double &blue)
{
	//_criticalSection.Lock();

	red = _saturationRed;
	green = _saturationGreen;
	blue = _saturationBlue;

	//_criticalSection.Unlock();
}

void CameraData::LaplacianBlend(Mat& mosaic, const Mat& image, const Mat& mask)
{
	_laplacianBlending.blend(image, mosaic, mask, mosaic);
}

void CameraData::DoWarp(Mat &imgLarge, Mat &imgMaskLarge, Mat &imgWarp, Mat &imgWarpMask)
{
	Mat matWarp;

	if (_maskImageWarp.size() != imgWarpMask.size())
	{
		// Create the mask for the output mosaic
		GetMosaicMask(imgWarp.size(), imgWarpMask);

		_maskImageWarp = Mat::zeros(imgWarpMask.size(), CV_8UC1);
		_tps.warpImage(imgMaskLarge, _maskImageWarp, 0.01, INTER_AREA, BACK_WARP);


		// Draw the frame on the supplied mosaic
		imgWarpMask &= _maskImageWarp;

		int blendWidth = 0;
		if (_cameraType == eFront)
		{
			FindCrossHairRadius();

			blendWidth = _pMosaicData->GetBlendWidth();
			Mat temp1, temp2;
			erode(imgWarpMask, temp1, Mat(), Point(-1, -1), blendWidth/2, 1, 1);
			blur(temp1, temp2, Size(blendWidth, blendWidth));
			temp2.copyTo(imgWarpMask);
		}

		// Find the bounding box for ROI processing
		Mat binary, points;
		threshold(imgWarpMask, binary, 0, 255, THRESH_BINARY);
		findNonZero(binary, points);
		_rectImageBorder = boundingRect(points);
		_rectImageWarp = _rectImageBorder;
		_rectImageWarp.x -= 2 * blendWidth;
		_rectImageWarp.y -= 2 * blendWidth;
		_rectImageWarp.width += 4 * blendWidth;
		_rectImageWarp.height += 4 * blendWidth;
		
		// Make sure in image
		_rectImageWarp.x = max(0, _rectImageWarp.x);
		_rectImageWarp.y = max(0, _rectImageWarp.y);

		if (_rectImageWarp.br().x > imgWarpMask.cols - 4)
			_rectImageWarp.width = (imgWarpMask.cols - _rectImageWarp.x) - 4;

		if (_rectImageWarp.br().y > imgWarpMask.rows - 4)
			_rectImageWarp.height = (imgWarpMask.rows - _rectImageWarp.y) - 4;

		// Make the width a multiple of 4
		if (_rectImageWarp.width % 4)
			_rectImageWarp.width += (4 - (_rectImageWarp.width % 4));

		// Make the height a multiple of 4
		if (_rectImageWarp.height % 4)
			_rectImageWarp.height += (4 - (_rectImageWarp.height % 4));
	}

	// Warp back to image space for stats
	Rect cameraROI = GetCameraROI();
	if (_imgMaskFromMosaic.size() != cameraROI.size())
	{
		Rect frontRect = _pMosaicData->GetFrontBorderRect();

		if (_cameraType == eFront)
		{
			_imgMaskFromMosaic = Mat::ones(cameraROI.size(), CV_8U);
		}
		else if (frontRect.area() != 0)
		{
			// Warp back to image space for stats
			Mat tempMosaicMask = imgWarpMask.clone();
			tempMosaicMask(frontRect).setTo(0);
			Mat tempImageMask;
			_tps.warpImage(tempMosaicMask, tempImageMask, 0.01, INTER_AREA, FORWARD_WARP);
			Rect cameraROI = GetCameraROI();
			Mat roi = tempImageMask(Rect(0, 0, cameraROI.width, cameraROI.height));
			_imgMaskFromMosaic = Mat::zeros(cameraROI.size(), CV_8U);
			roi.copyTo(_imgMaskFromMosaic);

			/*stringstream ss;
			ss << "C:\\NGE\\" << CameraNames[_cameraType] << "_MaskMosaci.bmp";
			imwrite(ss.str().c_str(), tempMosaicMask);
			ss = stringstream();
			ss << "C:\\NGE\\" << CameraNames[_cameraType] << "_MaskTemp.bmp";
			imwrite(ss.str().c_str(), tempImageMask);
			ss = stringstream();
			ss << "C:\\NGE\\" << CameraNames[_cameraType] << "_MaskFromMosaic.bmp";
			imwrite(ss.str().c_str(), _imgMaskFromMosaic);
			ss = stringstream();
			ss << "C:\\NGE\\" << CameraNames[_cameraType] << "_imgTempMask.bmp";
			imwrite(ss.str().c_str(), _imgTempMask);*/
		}
	}

	// create thin plate spline object and put the vectors into the constructor
	_tps.warpImage(imgLarge, imgWarp, 0.01, INTER_AREA, BACK_WARP);
}

void CameraData::FindCrossHairRadius()
{
	double dist = hypot(_maskImageWarp.rows / 2, _maskImageWarp.cols / 2);
	double slopeX = _maskImageWarp.cols / (2 * dist);
	double slopeY = _maskImageWarp.rows / (2 * dist);
	for (int i = 0; i < dist; i++)
	{
		Point loc = Point2f(i * slopeX, i * slopeY);
		Scalar val = _maskImageWarp.at<byte>(loc);
		if (val[0] > 0)
		{
			double radius = hypot(i * slopeX - (_maskImageWarp.cols / 2), i * slopeY - (_maskImageWarp.rows / 2));
			_pMosaicData->SetCrossHairRadius(2*radius / _maskImageWarp.rows);
			break;
		}
	}
}

void CameraData::LoadRawImage(string filename)
{
	bool bExists = false;
	FILE *file = fopen(filename.c_str(), "rb");
	if (file)
	{
		bExists = true;
		fclose(file);
	}

	if (bExists)
	{
		_imgRaw = imread(filename);
		vector<Mat> layers;
		split(_imgRaw, layers);
		int bayerOp = _cameraSettings.GetBayerType() + CV_BayerBG2BGR;
		cv::cvtColor(layers[0], _imgRaw, bayerOp);


		cv::Mat image = GetCameraImage();
		char fname[_MAX_PATH];
		sprintf_s(fname, "CameraImage_%s.bmp", GetName().c_str());
		cv::imwrite(fname, image);

		_bSimType = eRaw;
	}

	if (_bSimType == eNone)
	{
		string prefix = "RawImage_";
		int pos = filename.find(prefix);
		//string fname2 = filename.substr(0, pos) + filename.substr(pos + prefix.size());
		string fname2 = filename.substr(0, pos) + GetName() + ".png";
		cv::Mat imgRead = imread(fname2);

		if (imgRead.rows > 0)
		{
			cv::resize(imgRead, _imgRaw, _imgRaw.size());
			_bSimType = eRaw;
		}
	}
}

void CameraData::LoadBayerImage(string filename)
{
	FILE *file = fopen(filename.c_str(), "rb");
	if (file)
	{
		Size sensorSize = _cameraSettings.GetSensorSize();
		Mat img16 = Mat(sensorSize, CV_16U);
		fread(img16.data, 2, sensorSize.width * sensorSize.height, file);

		Size upSampleSize = Size(sensorSize.width * _upsampleFactor, sensorSize.height * _upsampleFactor);
		_imgRaw = Mat::zeros(upSampleSize, CV_8UC3);

		SetBayerImage(img16, false, "");
		_bSimType = eBayer;

		fclose(file);
	}
}

void CameraData::SetBayerImage(Mat img, bool bSaveData, string sDebugFolder)
{
	//LOG4CXX_INFO(logger, "SetCameraImage " << CameraNames[(int)_cameraType]);

	if (_ignoreFrameCount > 0)
	{
		LOG4CXX_DEBUG(logger, "Skipping frame with ignore count = " << _ignoreFrameCount);
		_ignoreFrameCount--;
		return;
	}

	_frameCount++;

	_criticalSectionImage.Lock();

	try
	{
		if (_bFPNCalActive && !_bAbortFPNCal)
		{
			Mat imgFloat = Mat(img.size(), CV_32F);
			img.convertTo(imgFloat, CV_32F);
			_imgFPNAvg += imgFloat;
			_nFPNAvgCount++;

			if (_nFPNAvgCount == _cameraSettings.GetFPNCalFrameCount())
			{
				_imgFPNAvg /= _cameraSettings.GetFPNCalFrameCount();
				_imgFPNAvg.convertTo(_imgFPNCal[_gainAutoCurrentIndex], CV_16U);

				_gainAutoCurrentIndex++;
				if (_gainAutoCurrentIndex == _gainAutoValues.size())
				{
					CompleteFPNCal();
				}
				else
				{
					NextFPNGain();
				}
			}
		}
		else if (_bSimType == eNone)
		{
			if (_imgBayer.size() != img.size())
			{
				_imgBayer = Mat::zeros(img.size(), CV_8U);
			}
			if (_imgScale.size() != img.size())
			{
				_imgScale = Mat::ones(img.size(), CV_32F);
				UpdateScaleImage(0.6, 0.5);
			}

			img -= _imgFPNCal[_gainAutoCurrentIndex];

#ifdef USE_USB_CAMERA 
			
			// Fix any dead pixels
			vector<Point> deadPixels = _cameraSettings.GetDeadPixels();
			vector<Point>::iterator it;
			for (it = deadPixels.begin(); it != deadPixels.end(); ++it)
			{
				if ((it->x >= 2) && (it->x < (img.size().width - 2))
					&& (it->y >= 2) && (it->y < (img.size().height - 2)))
				{
					double sum = img.at<ushort>(it->y, it->x - 2) + img.at<ushort>(it->y, it->x + 2)
						+ img.at<ushort>(it->y -2 , it->x) + img.at<ushort>(it->y + 2, it->x + 2);
					img.at<ushort>(*it) = (ushort)(sum / 4.0 + 0.5);
				}
			}
			

			double scale = 1.0 / (1 << 7);
			
			// Do temporal averaging
			CEnhancementSettings *pSettings = _pEnhancementControl->GetCurrentSettings();
			int size = pSettings->GetTemporalAveraging();
			if ((_cameraType == eFront)
				|| !_pEnhancementControl->IsEnhancementEnabled()
				|| !pSettings->IsTemporalAveragingEnabled())
			{
				size = 0;
			}
			if (size > 1)
			{
				// Size has changed so update queues
				if (size != _temporalAverageQueueSize)
				{
					_temporalAverageQueueSize = size;
					_temporalAverageQueue.clear();
					_temporalAverageScratch.clear();
					for (int i = 0; i < size; i++)
					{
						_temporalAverageScratch.push_back(Mat::zeros(img.size(), CV_16U));
					}
					_temporalAverageSum = Mat::zeros(img.size(), CV_16U);
				}

				// If the queue is full remove the last, subtract from sum, and add it back to scratch
				if (_temporalAverageQueue.size() == size)
				{
					Mat firstFrame = _temporalAverageQueue.front();
					_temporalAverageQueue.pop_front();
					_temporalAverageSum -= firstFrame;
					_temporalAverageScratch.push_back(firstFrame);
				}

				// Copy the incoming data to a scratch frame, put it in queue, and add to sum image
				Mat newFrame = _temporalAverageScratch.front();
				_temporalAverageScratch.pop_front();
				newFrame = img * scale;
				_temporalAverageSum += newFrame;
				_temporalAverageQueue.push_back(newFrame);

				if (pSettings->GetTemporalMedianAveraging())
				{
					for (int row = 0; row < img.rows; row++)
					{
						for (int col = 0; col < img.cols; col++)
						{
							Point pt = Point(col, row);
							Mat pixelVector = Mat::zeros(1, _temporalAverageQueue.size(), CV_16U);
							for (int i = 0; i < _temporalAverageQueue.size(); i++)
							{
								pixelVector.at<unsigned short>(i) = _temporalAverageQueue[i].at<unsigned short>(pt);
							}
							_imgBayer.at<byte>(pt) = round(median(pixelVector));
						}
					}
				}
				else
				{
					_temporalAverageSum.convertTo(_imgBayer, CV_8U, 1.0 / _temporalAverageQueue.size());
				}
			}
			else
			{
				bool gammaEnabled;
				float gammaValue;
				if (_cameraType == eFront)
				{
					gammaEnabled = _pEnhancementControl->GetCurrentSettings()->IsGammaEnabled();
					gammaValue = _pEnhancementControl->GetCurrentSettings()->GetGamma();
				}
				else
				{
					gammaEnabled = _pEnhancementControl->GetCurrentSettings()->IsGammaSideEnabled();
					gammaValue = _pEnhancementControl->GetCurrentSettings()->GetGammaSide();
				}

				if (gammaEnabled ||
					_pEnhancementControl->GetCurrentSettings()->IsBrightnessEnabled())
				{
					float gamma = gammaEnabled ? gammaValue : 1.0f;
					float brightness = _pEnhancementControl->GetCurrentSettings()->IsBrightnessEnabled() ?
						_pEnhancementControl->GetCurrentSettings()->GetBrightness() : 0.0f;

					if ((_fGamma != gamma) || (_fBrightness != brightness))
					{
						UpdateLookup(brightness, gamma);
					}

					Mat tmp = Mat(img.size(), CV_8U);
					ApplyLookup(img, tmp);
		
					// Apply front camera scaling to boost "gray areas"
					if ((_cameraType == eFront) && _pEnhancementControl->GetCurrentSettings()->IsFrontScaleEnabled())
					{
						float slope = _pEnhancementControl->GetCurrentSettings()->GetFrontScaleSlope();
						
						if ((_fScaleSlope != slope))
						{
							UpdateFrontScaleImage(slope);
						}
						multiply(tmp, _imgScale, _imgBayer, 1.0, CV_8U);
					}
					else
					{
						tmp.copyTo(_imgBayer);
					}
				}
				else
				{
					img.convertTo(_imgBayer, CV_8U, scale);
				}
			}
			
#else
			img.copyTo(_imgBayer);
#endif
			
			if (bSaveData)
			{
				SaveDebugImages(sDebugFolder);
			}
		}

		_framesPerSecond.NewFrame();

		// Change camera state to running if FPS is high enough
		if ((_cameraState != eRunning)
			&& (_framesPerSecond.CalcFPS() > 10))
		{
			LOG4CXX_INFO(logger, "Changing camera state to running for camera " << CameraNames[(int)_cameraType]);
			_cameraState = eRunning;
		}
	}
	catch (...)
	{
	}

	_criticalSectionImage.Unlock();

	SetEvent(_hWarpEvent);
}

void CameraData::SaveDebugImages(string sDebugFolder)
{
	string filebase = sDebugFolder + "\\" + GetName();
	string fileBayer = filebase + "_bayer.png";
	imwrite(fileBayer, _imgBayer);

	string fileRGB = filebase + "_rgb.png";
	int bayerOp = _cameraSettings.GetBayerType() + CV_BayerBG2BGR;
	Mat imgRaw;
	cv::cvtColor(_imgBayer, imgRaw, bayerOp);
	Rect roiCamera = GetCameraROI();
	Mat imgCameraRaw = imgRaw(roiCamera);
	imwrite(fileRGB, imgCameraRaw);

	string fileRGBBalanced = filebase + "_rgbBalanced.png";
	Mat camMask = _imgTempMask(Rect(Point(0, 0), roiCamera.size()));
	Mat balanced;
	WhiteBalance(imgCameraRaw, balanced, camMask);
	imwrite(fileRGBBalanced, balanced);

	if (_pEnhancementControl->IsEnhancementEnabled())
	{
		string fileRGBEnhanced = filebase + "_rgbEnhanced.png";
		Mat enhanced;
		_pEnhancementControl->EnhanceImage(balanced, enhanced);
		imwrite(fileRGBEnhanced, enhanced);
	}
}

Mat CameraData::GetCameraImage()
{
	Mat camImage;

	//LOG4CXX_INFO(logger, "GetCameraImage " << CameraNames[(int)_cameraType]);

	try
	{
		Rect roiCamera = GetCameraROI();
		camImage = _imgRaw(roiCamera).clone();
	}
	catch (...)
	{
	}

	return camImage;
}

void CameraData::WarpImage(cv::Mat &imgMosaic, cv::Mat &maskMosaic, bool bPickPoints)
{
	TimingData timingData, timingDataAll;
	string outfile;
	string imageFile = CameraNames[(int)_cameraType];
	
	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpImage_LoadImage", timingData.Elapsed_ms());
	timingData.Restart();

	// Create a roi of the image area
	Rect roiCamera = GetCameraROI();
	Mat imgCameraRaw = _imgRaw(roiCamera);
	
	// Do white balance on the roi
	Mat imgCameraBalanced;
	Mat statsMask = _imgMaskFromMosaic;
	if (statsMask.size().area() == 0)
	{
		statsMask = _imgTempMask(Rect(Point(0, 0), roiCamera.size()));
	}
	WhiteBalance(imgCameraRaw, imgCameraBalanced, statsMask);

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpImage_WhiteBalance", timingData.Elapsed_ms());

	// copy image to mosaic size image
	Mat roi = _imgTempImage(Rect(Point(0, 0), roiCamera.size()));

	// Do Contrast enhancement
	Mat camMask = _imgTempMask(Rect(Point(0, 0), roiCamera.size()));
	Mat enhanced = imgCameraBalanced.clone();
	if (_pEnhancementControl->IsEnhancementEnabled())
	{
		Mat enhanced;
		_pEnhancementControl->EnhanceImage(imgCameraBalanced, enhanced);
		enhanced.copyTo(roi, camMask);
	}
	else
	{
		imgCameraBalanced.copyTo(roi, camMask);
	}

	timingData.Restart();

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpImage_LargeImageCreation", timingData.Elapsed_ms());
	timingData.Restart();

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + imageFile + " Unbalanced.png";
		imwrite(outfile.c_str(), imgCameraRaw);

		outfile = _saveFolder + "\\" + imageFile + " Balanced.png";
		imwrite(outfile.c_str(), imgCameraBalanced);

		outfile = _saveFolder + "\\" + imageFile + " Enhanced.png";
		imwrite(outfile.c_str(), enhanced);
	}

	if (bPickPoints)
	{
		// Select the points on windows
		vector<Point2f> inputPoints = _cameraSettings.GetInputPoints();
		for (int i = 0; i < inputPoints.size();)
		{
			indexPoint = i;
			DrawWarpedImages();

			// Wait until user press some key
			int key;
			do {
				key = waitKey(0);
				cout << "Key pressed - " << key << endl;
			} while ((key != 110) && (key != 112) && (key != -1));
			if (key == 110) // 'n' for next
				i++;
			else if ((key == 112) && (i > 0)) // 'p' for previouse
				i--;
		}
	}

	DoWarp(_imgTempImage, _imgTempMask, imgMosaic, maskMosaic);

	// Do the side image scaling
	if ((_cameraType != eFront) && _pEnhancementControl->GetCurrentSettings()->IsSideScaleEnabled())
	{
		float offset = _pEnhancementControl->GetCurrentSettings()->GetSideScaleOffset();
		if (((_frameCount % DYNAMIC_SIDE_SCALE_UPDATE) == 0) 
			|| (_fScaleOffset != offset)
			|| (_imgScaleDynamic.size() != _rectImageWarp.size()))
		{
			UpdateDynamicScaleImage(imgMosaic, offset);
		}
		
		Mat mosaicRoi = imgMosaic(_rectImageWarp);
		multiply(mosaicRoi, _imgScaleDynamic, mosaicRoi, 1.0, CV_8UC3);
	}

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + imageFile + " Temp Image.png";
		imwrite(outfile.c_str(), _imgTempImage);

		outfile = _saveFolder + "\\" + imageFile + " Temp Mask.png";
		imwrite(outfile.c_str(), _imgTempMask);

		outfile = _saveFolder + "\\" + imageFile + " Warped.png";
		imwrite(outfile.c_str(), imgMosaic);

		outfile = _saveFolder + "\\" + imageFile + " Warped Mask.png";
		imwrite(outfile.c_str(), maskMosaic);
	}

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpImage_DoWarp", timingData.Elapsed_ms());
	timingData.Restart();
}

void CameraData::UpdateMosaic(Mat &mosaic, bool blend)
{
	if (_imgMosaic.rows == 0)
		return;

	_criticalSectionMosaic.Lock();

	TimingData timingData;
	
	// Blend the results into the mosaic
	if (blend)
	{
		Mat roiMosaic1 = mosaic(_rectImageWarp);
		Mat roiMosaic2 = _imgMosaic(_rectImageWarp);
		Mat roiMask = _maskMosaic(_rectImageWarp);

		LaplacianBlend(roiMosaic1, roiMosaic2, roiMask);
	}
	else
	{
		_imgMosaic.copyTo(mosaic, _maskMosaic);
	}

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "UpdateMosaic_Blending", timingData.Elapsed_ms());

	_criticalSectionMosaic.Unlock();
}

Rect CameraData::GetCameraROI()
{
	Size2f imageSize = _cameraSettings.GetImageSize();
	imageSize.width *= _upsampleFactor;
	imageSize.height *= _upsampleFactor;
	Point2f imageCenter = _cameraSettings.GetImageCenter();
	imageCenter.x *= _upsampleFactor;
	imageCenter.y *= _upsampleFactor;
	Rect roiCamera = Rect(Point(imageCenter.x - imageSize.width / 2, imageCenter.y - imageSize.height / 2),
		imageSize);
	if ((roiCamera.x < 0) || (roiCamera.y < 0) ||
		(roiCamera.br().x > _imgRaw.cols) || (roiCamera.br().y > _imgRaw.rows))
	{
		roiCamera = Rect(Point(_imgRaw.cols / 2 - imageSize.width / 2, _imgRaw.rows / 2 - imageSize.height / 2),
			imageSize);
	}

	return roiCamera;
}


void CameraData::DrawCloverImage(cv::Mat &imgMosaic, cv::Mat &maskMosaic)
{
	TimingData timingData, timingDataAll;
	string outfile;
	string imageFile = CameraNames[(int)_cameraType];

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "DrawCloverImage_LoadImage", timingData.Elapsed_ms());
	timingData.Restart();

	// Create a roi of the image area
	Rect roiCamera = GetCameraROI();
	Mat imgCameraRaw = _imgRaw(roiCamera);

	// Do white balance on the roi
	Mat imgCameraBalanced;
	Mat camMask = _imgTempMask(Rect(Point(0, 0), roiCamera.size()));
	WhiteBalance(imgCameraRaw, imgCameraBalanced, camMask);

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "DrawUnstichedImage_WhiteBalance", timingData.Elapsed_ms());
	timingData.Restart();

	// Do Contrast enhancement
	Mat enhanced = imgCameraBalanced.clone();
	if (_pEnhancementControl->IsEnhancementEnabled())
	{
		Mat enhanced;
		_pEnhancementControl->EnhanceImage(imgCameraBalanced, enhanced);
		imgCameraBalanced = enhanced.clone();
	}

	// Determine the roi in the mosaic
	int mosaicDim = min(imgMosaic.cols, imgMosaic.rows);
	Point mosaicCenter = Point(imgMosaic.cols / 2, imgMosaic.rows / 2);
	int sizeSide = (int)((float)_pMosaicData->GetSideCameraScale()*mosaicDim);
	int offsetSide = (int)((float)_pMosaicData->GetSideCameraOffset()*mosaicDim);

	Rect roiMosaic;
	double angle = _cameraSettings.GetCameraAngle();
	switch (_cameraType)
	{
	case eTop:
		roiMosaic = Rect(mosaicCenter.x - sizeSide / 2, mosaicCenter.y - offsetSide / 2 - sizeSide,
			sizeSide, sizeSide);

		break;
	case  eLeft:
		roiMosaic = Rect(mosaicCenter.x - offsetSide / 2 - sizeSide, mosaicCenter.y - sizeSide / 2,
			sizeSide, sizeSide);
		break;
	case  eBottom:
		roiMosaic = Rect(mosaicCenter.x - sizeSide / 2, mosaicCenter.y + offsetSide / 2,
			sizeSide, sizeSide);
		break;
	case eRight:
		roiMosaic = Rect(mosaicCenter.x + offsetSide / 2, mosaicCenter.y - sizeSide / 2,
			sizeSide, sizeSide);
		break;
	}

	Mat imgRoiCopy = Mat(roiMosaic.size(), CV_8UC3);
	resize(imgCameraBalanced, imgRoiCopy, roiMosaic.size());

	// Rotate image to mosaic size image
	Mat rot_mat = getRotationMatrix2D(Point2f(roiMosaic.width / 2, roiMosaic.height / 2), angle, 1.0);

	// Translate to the correct position
	rot_mat.at<double>(0, 2) += roiMosaic.x;
	rot_mat.at<double>(1, 2) += roiMosaic.y;

	// Transform the image
	warpAffine(imgRoiCopy, imgMosaic, rot_mat, imgMosaic.size());

	// Transform the mask
	Mat maskRoiCopy = Mat::zeros(roiMosaic.size(), CV_8U);
	int radiusSide = (int)((float)_pMosaicData->GetSideCameraRadius()*roiMosaic.height/2);
	circle(maskRoiCopy, Point(roiMosaic.width / 2, roiMosaic.height / 2), radiusSide, Scalar(255), -1);
	warpAffine(maskRoiCopy, maskMosaic, rot_mat, maskMosaic.size());

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + imageFile + " Unbalanced.png";
		imwrite(outfile.c_str(), imgCameraRaw);

		outfile = _saveFolder + "\\" + imageFile + " Balanced.png";
		imwrite(outfile.c_str(), imgCameraBalanced);
	}

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "DrawUnstichedImage_DoCopy", timingData.Elapsed_ms());
	timingData.Restart();

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "DrawUnstichedImage__Total", timingDataAll.Elapsed_ms());
}



void CameraData::DrawUnstichedImage(cv::Mat &imgMosaic, cv::Mat &maskMosaic, float ratioFront, float ratioSide)
{
	TimingData timingData, timingDataAll;
	string outfile;
	string imageFile = CameraNames[(int)_cameraType];

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "DrawUnstichedImage_LoadImage", timingData.Elapsed_ms());
	timingData.Restart();

	// Create a roi of the image area
	Rect roiCamera = GetCameraROI();
	Mat imgCameraRaw = _imgRaw(roiCamera);

	// Do white balance on the roi
	Mat imgCameraBalanced;
	Mat camMask = _imgTempMask(Rect(Point(0, 0), roiCamera.size()));
	WhiteBalance(imgCameraRaw, imgCameraBalanced, camMask);

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "DrawUnstichedImage_WhiteBalance", timingData.Elapsed_ms());
	timingData.Restart();

	// Do Contrast enhancement
	Mat enhanced = imgCameraBalanced.clone();
	if (_pEnhancementControl->IsEnhancementEnabled())
	{
		Mat enhanced;
		_pEnhancementControl->EnhanceImage(imgCameraBalanced, enhanced);
		imgCameraBalanced = enhanced.clone();
	}

	// Determine the roi in the mosaic
	int mosaicDim = min(imgMosaic.cols, imgMosaic.rows);
	Point mosaicCenter = Point(imgMosaic.cols / 2, imgMosaic.rows / 2);
	int sizeFront = (int)((float)ratioFront*mosaicDim);
	int sizeSide = (int)((float)ratioSide*mosaicDim);

	Rect roiMosaic;
	double angle = 0;
	switch (_cameraType)
	{
	case eTop:
		roiMosaic = Rect(mosaicCenter.x - sizeSide / 2, mosaicCenter.y - sizeFront / 2 - sizeSide,
			sizeSide, sizeSide);
		angle = -90;
		break;
	case  eLeft:
		roiMosaic = Rect(mosaicCenter.x - sizeFront / 2 - sizeSide, mosaicCenter.y - sizeSide / 2,
			sizeSide, sizeSide);
		angle = 0;
		break;
	case  eBottom:
		
		roiMosaic = Rect(mosaicCenter.x - sizeSide / 2, mosaicCenter.y + sizeFront / 2,	sizeSide, sizeSide);  
		angle = 90;

		/* javqui test
		roiMosaic = Rect(mosaicCenter.x - 200, mosaicCenter.y - 200, 400, 400);
		angle = 0;
		*/
		break;
	case eRight:
		roiMosaic = Rect(mosaicCenter.x + sizeFront / 2, mosaicCenter.y - sizeSide / 2,
			sizeSide, sizeSide);
		angle = 180;
		break;
	case eFront:
		roiMosaic = Rect(mosaicCenter.x - sizeFront / 2, mosaicCenter.y - sizeFront / 2,
			sizeFront, sizeFront);
		angle = 0;
		break;
	}
	
	Mat imgRoiCopy = Mat(roiMosaic.size(), CV_8UC3);
	Mat imgMaskCopy = Mat(roiMosaic.size(), CV_8U);
	resize(imgCameraBalanced, imgRoiCopy, roiMosaic.size());
	resize(camMask, imgMaskCopy, roiMosaic.size());

	// Rotate image to mosaic size image
	Mat rot_mat = getRotationMatrix2D(Point2f(roiMosaic.width / 2, roiMosaic.height / 2), angle, 1.0);

	/// Rotate the warped image
	warpAffine(imgRoiCopy, imgRoiCopy, rot_mat, imgRoiCopy.size());

	Mat imgMosaciRoi = imgMosaic(roiMosaic);
	imgRoiCopy.copyTo(imgMosaciRoi, imgMaskCopy);

	Mat maskMosaciRoi = maskMosaic(roiMosaic);
	maskMosaciRoi.setTo(1);

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + imageFile + " Unbalanced.png";
		imwrite(outfile.c_str(), imgCameraRaw);

		outfile = _saveFolder + "\\" + imageFile + " Balanced.png";
		imwrite(outfile.c_str(), imgCameraBalanced);
	}

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "DrawUnstichedImage_DoCopy", timingData.Elapsed_ms());
	timingData.Restart();

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "DrawUnstichedImage__Total", timingDataAll.Elapsed_ms());
}

#if 0

void CameraData::MoveTiePoint(Point2f before, Point2f after, Size mosaicSize)
{
	Point2f mosaicCenter = Point2f(mosaicSize.width / 2, mosaicSize.height / 2);
	double mosaicDim = min(mosaicSize.width / 2, mosaicSize.height / 2);
	Point2f cartBefore = GetCartPointFromLogPolar(before);
	cartBefore -= mosaicCenter;
	cartBefore.x /= mosaicDim;
	cartBefore.y /= mosaicDim;

	Point2f cartAfter = GetCartPointFromLogPolar(after);
	cartAfter -= mosaicCenter;
	cartAfter.x /= mosaicDim;
	cartAfter.y /= mosaicDim;

	// Find points along the same ray
	vector<int> indexes;
	double angleLimit = 30.0 * PI / 180;
	for (int i = 0; i < _outputPoints.size(); i++)
	{
		double pointAngle1 = atan2(_outputPoints[i].y, _outputPoints[i].x);
		double pointAngle2 = atan2(cartBefore.y, cartBefore.x);
		if (fabs(pointAngle1 - pointAngle2) < angleLimit)
		{
			indexes.push_back(i);
		}
	}

	// Find the closes point to adjust
	double minDist = FLT_MAX;
	int minDistIndex = -1;
	for (int i = 0; i < indexes.size(); i++)
	{
		Point2f point = _outputPoints[indexes[i]];
		double dist = sqrt(pow(point.x - cartBefore.x, 2) + pow(point.y - cartBefore.y, 2));
		if (dist < minDist)
		{
			minDist = dist;
			minDistIndex = indexes[i];
		}
	}

	Point2f offset = cartAfter - cartBefore;
	_outputPoints[minDistIndex] += offset;

	UpdateThinPlateSpline();
}

void CameraData::WarpEdges(string filename, Mat &mosaic)
{
	TimingData timingData, timingDataAll;
	string outfile, file(filename);

	size_t pos1 = file.rfind('\\');
	string outputFolder = file.substr(0, pos1) + "\\Output";
	size_t pos2 = file.rfind('.');
	string imageFile = file.substr(pos1+1, pos2-(pos1+1));
	
	Mat imgRaw = imread(filename);

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpEdges_LoadImage", timingData.Elapsed_ms());
	timingData.Restart();

	// Warp the green channel with the current settings
	vector<Mat> layers;
	split(imgRaw, layers);

	// Create the mask for the camera image
	imgMaskLarge = Mat::zeros(mosaic.size(), CV_8UC1);
	GetCameraMask(mosaic.size(), imgMaskLarge);

	// copy the green image to mosaic size image
	Mat imgLarge = Mat::zeros(mosaic.size(), CV_8U);
	Mat roi = imgLarge(Rect(0, 0, imgRaw.cols, imgRaw.rows));
	layers[1].copyTo(roi);

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpEdges_ExtractGreenChannel", timingData.Elapsed_ms());
	timingData.Restart();

	// Create the mask for the output mosaic
	imgMaskMosaic = Mat::zeros(mosaic.size(), CV_8UC1);
	GetMosaicMask(imgLarge.size(), imgMaskMosaic);

	// Warp the image and mask
	Mat imgWarp, imgWarpMask;
	DoWarp(imgLarge, imgMaskLarge, imgWarp, imgWarpMask);

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpEdges_DoWarp", timingData.Elapsed_ms());
	timingData.Restart();

	// Find the edges
	Mat imgWarpCropped;
	FindEdgeVectors(imgWarp, imgWarpMask, imgWarpCropped);

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpEdges_FindEdgeVectors", timingData.Elapsed_ms());
	timingData.Restart();

	// Combine the edges with the supplied mosaic
	mosaic |= imgWarpCropped;

	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpEdges_CombineWithMosaic", timingData.Elapsed_ms());
	_pProcTimes->AddTime(CameraNames[(int)_cameraType], "WarpEdges__Total", timingDataAll.Elapsed_ms());
}

double CameraData::GetTopAngle()
{
	double angle = 360.0 - GetAngle() - 45.0;

	// Handle the wrap around
	if (angle >= 360)
		angle = (angle - 360);
	if (angle < 0)
		angle = (360 + angle);

	return angle;
}

double CameraData::GetBottomAngle()
{
	double angle = 360.0 - GetAngle() + 45.0;

	// Handle the wrap around
	if (angle >= 360)
		angle = (angle - 360);
	if (angle < 0)
		angle = (360 + angle);

	return angle;
}



Point2f CameraData::GetCartPointFromLogPolar(Point2f point)
{
	double rad = point.y*PI / 180.0;
	double x = exp(point.x / _logPolarM) * cos(rad);
	double y = exp(point.x / _logPolarM) * sin(rad);

	return Point2f(_logPolarCenter.x + x, _logPolarCenter.y + y);
}

void CameraData::FindEdgeVectors(Mat &mosaic, Mat &mosaicMask, Mat &imgWarpCropped)
{
	_logPolarCenter = Point2f(mosaic.size().width / 2, mosaic.size().height / 2);
	_logPolarM = 100.0;
	
	CvPoint2D32f center;
	center.x = _logPolarCenter.x;
	center.y = _logPolarCenter.y;
	Mat dstEdges(mosaic.size(), mosaic.type());
	CvMat cvMosaic = mosaic;
	CvMat cvMosaicInv = dstEdges;
	cvLogPolar(&cvMosaic, &cvMosaicInv, center, _logPolarM, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS);

	Mat dstMask(mosaic.size(), mosaic.type());
	cvMosaic = mosaicMask;
	cvMosaicInv = dstMask;
	cvLogPolar(&cvMosaic, &cvMosaicInv, center, 100.0, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS);
	
	string outfile;
	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + CameraNames[(int)_cameraType] + " 1 Mosaic Image.png";
		imwrite(outfile.c_str(), mosaic);

		outfile = _saveFolder + "\\" + CameraNames[(int)_cameraType] + " 2 Polar Image.png";
		imwrite(outfile.c_str(), dstEdges);

		outfile = _saveFolder + "\\" + CameraNames[(int)_cameraType] + " 3 Polar Mask.png";
		imwrite(outfile.c_str(), dstMask);
	}

	// Run Canny edge detection on the image
	Mat detected_edges, src_gray;
	src_gray = dstEdges;
#if 0
	int lowThreshold = 200;
	int ratio = 3;
	int kernel_size = 5;
	blur(src_gray, src_gray, Size(5, 5));
	Canny(src_gray, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size);
	dstEdges = detected_edges;
#else
	Mat grad_x, gradThresh;
	int scale = 1.0;
	int delta = 0;
	int kernel_size = 3;
	int ddepth = CV_16S;
	medianBlur(src_gray, src_gray, 3);

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + CameraNames[(int)_cameraType] + " 4 Polar Smoothed.png";
		imwrite(outfile.c_str(), src_gray);
	}

	//blur(src_gray, src_gray, Size(5, 5));
	Sobel(src_gray, grad_x, ddepth, 1, 0, kernel_size, scale, delta, BORDER_DEFAULT);
	threshold(grad_x, gradThresh, 15, 65535, CV_8U);
	//Laplacian(src_gray, grad_x, ddepth, kernel_size, scale, delta, BORDER_DEFAULT);
	//threshold(grad_x, gradThresh, 150, 65535, CV_8U);
	gradThresh.convertTo(dstEdges, CV_8U);
	
#endif

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + CameraNames[(int)_cameraType] + " 5 Polar Edges.png";
		imwrite(outfile.c_str(), dstEdges);
	}

	// Erode the mask to eliminate edges
	erode(dstMask, dstMask, Mat(), Point(-1, -1), 5, 1, 1);
	imgWarpCropped = Mat::zeros(dstEdges.size(), CV_8U);
	dstEdges.copyTo(imgWarpCropped, dstMask);

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + CameraNames[(int)_cameraType] + " 6 Polar Edges Cropped.png";
		imwrite(outfile.c_str(), imgWarpCropped);
	}

	// Do a skeleton operation
	CThinning::thinning(imgWarpCropped);

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + CameraNames[(int)_cameraType] + " 7 Polar Edges Thinned.png";
		imwrite(outfile.c_str(), imgWarpCropped);
	}

	// Do joining
	int morph_size = 1;
	Mat element = getStructuringElement(MORPH_ELLIPSE, Size(2 * morph_size + 1, 2 * morph_size + 1), Point(morph_size, morph_size));
	morphologyEx(imgWarpCropped, imgWarpCropped, MORPH_CLOSE, element);

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + CameraNames[(int)_cameraType] + " 8 Polar Edges Joined.png";
		imwrite(outfile.c_str(), imgWarpCropped);
	}

	// Find the near vertical lines
	double topAngle = GetTopAngle();
	double bottomAngle = GetBottomAngle();

	double topBand = mosaic.rows * topAngle / 360.0;
	double bottomBand = mosaic.rows * bottomAngle / 360.0;

	int border = mosaic.rows * 30.0 / 360.0;
	int colStart = 450;
	int width = 150;
	Rect topRect = Rect(colStart, topBand, width, border);
	Rect bottomRect = Rect(colStart, bottomBand - border, width, border);

	// Find the vectors
	FindEdgeVectors(imgWarpCropped, topRect, _topPoints, _topVectors);
	FindEdgeVectors(imgWarpCropped, bottomRect, _bottomPoints, _bottomVectors);


	Mat dstEdgesRGB;
	cvtColor(imgWarpCropped, dstEdgesRGB, CV_GRAY2BGR);
	rectangle(dstEdgesRGB, topRect, Scalar(0, 0, 255), 2);
	rectangle(dstEdgesRGB, bottomRect, Scalar(0, 0, 255), 2);

	/// Draw vectors
	for (int i = 0; i < _topPoints.size(); i++)
		line(dstEdgesRGB, _topPoints[i] + _topVectors[i], _topPoints[i] - _topVectors[i], Scalar(0, 255, 0), 1);
	for (int i = 0; i < _bottomPoints.size(); i++)
		line(dstEdgesRGB, _bottomPoints[i] + _bottomVectors[i], _bottomPoints[i] - _bottomVectors[i], Scalar(0, 0, 255), 1);

	if (!_saveFolder.empty())
	{
		outfile = _saveFolder + "\\" + CameraNames[(int)_cameraType] + " 9 Edges Vectors.png";
		imwrite(outfile.c_str(), dstEdgesRGB);
	}

}

void CameraData::FindEdgeVectors(Mat &dstEdges, Rect roi, vector<Point2f> &points, vector<Point2f> &vectors)
{
	Mat imgRoi = dstEdges(roi).clone();

	// Clear any existing vectors
	points.clear();
	vectors.clear();

	/// Find contours
	vector<vector<Point> > contours;
	findContours(imgRoi, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	/// Find the rotated rectangles
	for (int i = 0; i < contours.size(); i++)
	{
		RotatedRect minRect = minAreaRect(Mat(contours[i]));
	
		Point2f vec = GetVector(minRect);
		double angle = atan2f(vec.y, vec.x) * 180.0/PI;

		if ((abs(abs(angle) - 90) > 45) || (abs(vec.y) < (roi.height * 0.25)))
			continue;

		points.push_back(minRect.center + Point2f(roi.tl()));
		vectors.push_back(vec);
	}
}

Point2f CameraData::GetVector(RotatedRect rotatedRect)
{
	if (rotatedRect.size.width < rotatedRect.size.height){
		double angleRad = (rotatedRect.angle - 90) * PI / 180.0;
		return Point2f(cos(angleRad) * rotatedRect.size.height / 2,
			sin(angleRad) * rotatedRect.size.height / 2);
	}
	else{
		double angleRad = rotatedRect.angle * PI / 180.0;
		return Point2f(cos(angleRad) * rotatedRect.size.width / 2,
			sin(angleRad) * rotatedRect.size.width / 2);
	}
}

void CameraData::GetTopEdgeVectors(vector<Point2f> &origins, vector<Point2f> &directions)
{
	origins = _topPoints;
	directions = _topVectors;
}

void CameraData::GetBottomEdgeVectors(vector<Point2f> &origins, vector<Point2f> &directions)
{
	origins = _bottomPoints;
	directions = _bottomVectors;
}



#endif

void CallBackFuncImage(int event, int x, int y, int flags, void* userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		x /= scaleImage;
		y /= scaleImage;

		((CameraData *)userdata)->UpdateImagePoint(indexPoint, Point(x, y) + imageOrigin);
		((CameraData *)userdata)->DrawWarpedImages();
	}
}

void CallBackFuncMosaic(int event, int x, int y, int flags, void* userdata)
{
	if (event == EVENT_LBUTTONDOWN)
	{
		((CameraData *)userdata)->UpdateMosaicPoint(indexPoint, Point(x, y));		
		((CameraData *)userdata)->DrawWarpedImages();
	}
}

void CameraData::UpdateImagePoint(int index, Point point)
{
	Point2f imageCenter = _cameraSettings.GetImageCenter();
	Size2f imageSize = _cameraSettings.GetImageSize();

	_cameraSettings.UpdateInputPoint(index, Point2f(2.0*(point.x - imageCenter.x) / imageSize.width,
		2.0*(point.y - imageCenter.y) / imageSize.height));

	UpdateThinPlateSpline();
}

void CameraData::UpdateMosaicPoint(int index, Point point)
{

	double mosaicDim = min(_pMosaicData->GetSize().width, _pMosaicData->GetSize().height);
	Point2f mosaicCenter = Point2f(_imgMosaic.cols / 2.0, _imgMosaic.rows / 2.0);
	_cameraSettings.UpdateOutputPoint(index, Point2f(2.0*(point.x - mosaicCenter.x) / mosaicDim,
		2.0*(point.y - mosaicCenter.y) / mosaicDim));

	UpdateThinPlateSpline();
}

void CameraData::DrawWarpedImages()
{
	destroyAllWindows();

	Mat imgOverlay = GetCameraAlignmentImage(scaleImage, indexPoint);
	
	namedWindow(imageName.c_str(), CV_WINDOW_NORMAL);
	setMouseCallback(imageName.c_str(), CallBackFuncImage, this);
	imshow(imageName.c_str(), imgOverlay);

	Mat mosaicOverlay = _imgMosaic.clone();
	UpdateMosaicAlignmentImage(mosaicOverlay, indexPoint);

	UpdateThinPlateSpline();
	DoWarp(_imgTempImage, _imgTempMask, _imgMosaic, _maskMosaic);

	
	namedWindow("Mosaic", CV_WINDOW_NORMAL);
	setMouseCallback("Mosaic", CallBackFuncMosaic, this);
	imshow("Mosaic", mosaicOverlay);
}

Mat CameraData::GetCameraAlignmentImage(double scale, int pointIndex)
{
	Mat imgOverlay;
	stringstream ssImage;

	vector<Point2f> inputPoints = _cameraSettings.GetInputPoints();
	Size2f imageSize = _cameraSettings.GetImageSize();
	Point2f imageCenter = _cameraSettings.GetImageCenter();

	Point locImage = Point((inputPoints[pointIndex].x + 1)*imageSize.width / 2,
		(inputPoints[pointIndex].y + 1)*imageSize.height / 2);
	ssImage << "Index= " << pointIndex << " Loc=(" << locImage.x << "," << locImage.y << ")";

	imageOrigin = Point(imageCenter.x - imageSize.width / 2, imageCenter.y - imageSize.height / 2);
	Rect rcRoi = Rect(Point(0, 0), imageSize);
	Mat imgRoi = _imgTempImage(rcRoi);
	resize(imgRoi, imgOverlay, Size(), scale, scale);

	circle(imgOverlay, Point((locImage.x - rcRoi.x)* scale, (locImage.y - rcRoi.y) * scale), 10, Scalar(0, 0, 255), 3);
	putText(imgOverlay, ssImage.str(), Point(10, imgOverlay.rows - 10), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3);

	return imgOverlay;
}

void CameraData::UpdateMosaicAlignmentImage(Mat &mosaicOverlay, int pointIndex)
{
	// Draw the frame on the supplied mosiac
	imgMosaicInput.copyTo(mosaicOverlay, imgMaskInput);
	_pMosaicData->DrawCrossHairs(mosaicOverlay);

	stringstream ssMosaic;
	vector<Point2f> outputPoints = _cameraSettings.GetOutputPoints();

	double mosaicDim = min(_pMosaicData->GetSize().width, _pMosaicData->GetSize().height);
	Point2f mosaicCenter = Point2f(mosaicOverlay.cols / 2.0, mosaicOverlay.rows / 2.0);
	Point locMosaic = Point(outputPoints[pointIndex].x * mosaicDim / 2 + mosaicCenter.x,
		outputPoints[pointIndex].y * mosaicDim / 2 + mosaicCenter.y);
	ssMosaic << "Index= " << pointIndex << " Loc=(" << locMosaic.x << "," << locMosaic.y << ")";
	circle(mosaicOverlay, locMosaic, 10, Scalar(0, 0, 255), 3);
	putText(mosaicOverlay, ssMosaic.str(), Point(10, _imgTempImage.rows - 10), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 3);
}

void CameraData::UpdateCameraView(Mat *pMosaicInput, Mat *pMaskInput, bool pickPoints)
{
	// Recreate mosais frame if size changed
	if (_imgMosaic.size() != _pMosaicData->GetSize())
	{
		_imgMosaic = Mat::zeros(_pMosaicData->GetSize(), CV_8UC3);
		_maskMosaic = Mat::zeros(_pMosaicData->GetSize(), CV_8UC1);
	}

	CvPoint2D32f center;
	center.x = (float)_imgMosaic.size().width / 2.0F;
	center.y = (float)_imgMosaic.size().height / 2.0F;

	if (_imgTempImage.size() != _imgMosaic.size())
	{
		_imgTempImage = Mat::zeros(_imgMosaic.size(), CV_8UC3);

		// Create the mask for the camera image
		_imgTempMask = Mat::zeros(_imgMosaic.size(), CV_8UC1);
		GetCameraMask(_imgMosaic.size(), _imgTempMask);
	}

	//_criticalSectionMosaic.Lock();
	switch (_eImageStitchingMode)
	{
	case eNormal:
	{
		bool bPickPoints = false;
		if (pMosaicInput && pMaskInput)
		{
			bPickPoints = pickPoints;
			imgMosaicInput = pMosaicInput->clone();
			imgMaskInput = pMaskInput->clone();
		}

		WarpImage(_imgMosaic, _maskMosaic, bPickPoints);
		break;
	}
	/*
	case eNoStitching:
		DrawUnstichedImage(_imgMosaic, _maskMosaic, 0.5, 0.25);
		break;
	*/
	case eClover:
		if (_cameraType == eFront)
		{
			WarpImage(_imgMosaic, _maskMosaic, false); 
		}
		else
		{
			DrawCloverImage(_imgMosaic, _maskMosaic);
		}
		break;
	case eFrontOnly:
		if (_cameraType == eFront)
		{
			 
			DrawUnstichedImage(_imgMosaic, _maskMosaic, 1.0, 0);  // original regresar javqui
			//DrawUnstichedImage(_imgMosaic, _maskMosaic, 0.2, 1.0);  // javqui
		}
		break;
	}

	_pMosaicData->SetCameraMosaic(_cameraType, _imgMosaic, _maskMosaic, _rectImageWarp, _rectImageBorder);
	//_criticalSectionMosaic.Unlock();

}

UINT CameraData::WarpLoop()
{
	ImageStitchingMode eStichingMode = _eImageStitchingMode;

	while (!_bExiting)
	{
		DWORD dwEvent = WaitForSingleObject(
			_hWarpEvent,           // number of objects in array
			500);

		// If upsample changed, reset settings
		if ((_cameraType != eFront) &&
			_upsampleFactor != _pEnhancementControl->GetCurrentSettings()->GetUpsampleFactor())
		{
			_ignoreFrameCount = 60;
			_upsampleFactor = _pEnhancementControl->GetCurrentSettings()->GetUpsampleFactor();
			RefreshCameraSettings();
		}

		if (dwEvent == WAIT_TIMEOUT)
		{
			continue;
		}
		
		ResetEvent(_hWarpEvent);

		// First frame after blacking out
		if (_cameraState == eBlank)
		{
			_cameraState = eInit;

			_imgBayer.setTo(0);

			SetCameraChip(_cameraSettings.GetCameraChip());
			int retVal = InitCameraChip(false);
			if (retVal == 0)
			{
				RefreshSensorSettings();
			}
			else
			{
				_cameraState = eNotPresent;
			}
		}
		else if (_cameraState != eRunning)
		{
			continue;
		}

		//_criticalSectionMosaic.Lock();

		try
		{			
			if (eStichingMode != _eImageStitchingMode)
			{
				eStichingMode = _eImageStitchingMode;
				ResetTempImages();
			}
			
			DebayerImage();

			UpdateCameraView();

			// Find the amount of saturation for this camera
			//vector<Mat> layers;
			//split(_imgMosaic, layers);
			//_saturationBlue = CalculateSaturation(layers[0], _maskMosaic);
			//_saturationGreen = CalculateSaturation(layers[1], _maskMosaic);
			//_saturationRed = CalculateSaturation(layers[2], _maskMosaic);

			SetEvent(_hCaptureEvent);
			//if (_cameraType == eFront)
			{
				SetEvent(_hUpdateEvent);
			}
		}
		catch (cv::Exception ex)
		{
			LOG4CXX_ERROR(logger, "Camera " + CameraNames[(int)_cameraType] + ": WarpLoop exception creating mosaic:" + ex.msg);
		}
		catch (...)
		{
			LOG4CXX_ERROR(logger, "Camera " + CameraNames[(int)_cameraType] + ": WarpLoop exception creating mosaic");
		}

		//_criticalSectionMosaic.Unlock();
	}

	LOG4CXX_INFO(logger, "WarpLoop exiting for " << CameraNames[(int)_cameraType]);

	return 0;
}


// calculates the median value of a single channel
// based on https://github.com/arnaudgelas/OpenCVExamples/blob/master/cvMat/Statistics/Median/Median.cpp
double CameraData::median(cv::Mat channel)
{
	double m = (channel.rows*channel.cols) / 2;
	int bin = 0;
	double med = -1.0;

	int histSize = 256;
	float range[] = { 0, 256 };
	const float* histRange = { range };
	bool uniform = true;
	bool accumulate = false;
	cv::Mat hist;
	cv::calcHist(&channel, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

	for (int i = 0; i < histSize && med < 0.0; ++i)
	{
		bin += cvRound(hist.at< float >(i));
		if (bin > m && med < 0.0)
			med = i;
	}

	// if never found it it must be saturated
	if (med < 0)
		med = 255;

	return med;
}

void CameraData::SetMeasurementType(Measurement type)
{ 
	LOG4CXX_INFO(logger, "SetMeasurementType for camera " << CameraNames[(int)_cameraType] 
		<< " to " << MeasurementNames[(int)type]);

	_ignoreFrameCount = max(_ignoreFrameCount, 10);

	_eMeasurementTypeNew = type; 
	if (_eMeasurementTypeNew == eNoMeasurement)
	{
		_eMeasurementType = eNoMeasurement;
	}
}
double CameraData::GetMeasurement()	
{
	LOG4CXX_INFO(logger, "GetMeasurement for camera " << CameraNames[(int)_cameraType] 
		<< " type " << MeasurementNames[(int)_eMeasurementType]
		<< " is " << _dMeasurementValue); 

	//LOG4CXX_INFO(logger, printf( "GetMeasurement for camera %s type %s is %.6f", CameraNames[(int)_cameraType],MeasurementNames[(int)_eMeasurementType], _dMeasurementValue));
	return _dMeasurementValue; 
}

void CameraData::DoMeasurement(Mat &_imgMosaic, Mat &_maskMosaic)
{
	if (_eMeasurementTypeNew != _eMeasurementType)
	{
		LOG4CXX_INFO(logger, "DoMeasurement for camera " << CameraNames[(int)_cameraType]
			<< " updating type to  " << MeasurementNames[(int)_eMeasurementTypeNew]);

		_eMeasurementType = _eMeasurementTypeNew;
		_dMeasurementCount = 0;

		switch (_eMeasurementType)
		{
		case eDeviation: _dMeasurementValue = DBL_MAX; break;
		case eLightResponse: _dMeasurementValue = DBL_MAX; break;
		}
	}

	int smoothWidth = 5;
	Mat smoothed, mask, mean3, mean1, stddev;

	switch (_eMeasurementType)
	{
	case eDeviation:
		// Smooth the image
		GaussianBlur(_imgMosaic, smoothed, Size(smoothWidth, smoothWidth), 1);

		// Erode the mask
		erode(_maskMosaic, mask, Mat(), Point(-1, -1), smoothWidth, 1, 1);

		// Find the StdDev
		meanStdDev(smoothed, mean3, stddev, mask);
		_dMeasurementValue = stddev.at<double>(0);
		break;
	case eLightResponse:
		meanStdDev(_imgMosaic, mean3, stddev, _maskMosaic);
		meanStdDev(mean3, mean1, stddev);
		_dMeasurementValue = min(_dMeasurementValue, mean1.at<double>(0));
		break;
	}
}

double CameraData::GetMaxFPNValue()
{
	double maxValAll = NAN;

	// Create a max to measure the max value inside of
	Mat mask = Mat::zeros(_cameraSettings.GetSensorSize(), CV_8U);
	int border = 10;
	rectangle(mask,
		Point(border, border),
		Point(mask.cols - border, mask.rows - border),
		Scalar(255, 255, 255), -1);

	for (int i = 0; i < _imgFPNCal.size(); i++)
	{
		if ((_imgFPNCal[i].size().height > 0) && (_imgFPNCal[i].size().width > 0))
		{
			if (isnan(maxValAll))
			{
				maxValAll = 0;
			}

			double minVal = NAN;
			double maxVal = NAN;
			Point minLocation, maxLocation;
			int smoothWidth = 3;
			Mat smoothed;
			GaussianBlur(_imgFPNCal[i], smoothed, Size(smoothWidth, smoothWidth), 1);
			minMaxLoc(smoothed, &minVal, &maxVal, &minLocation, &maxLocation, mask);
			if (_gainAutoValues[i] > 0)
			{
				maxVal /= (double)_gainAutoValues[i];
			}
			
			maxValAll = max(maxVal, maxValAll);
		}
	}

	return maxValAll;
}

vector<cv::Mat> CameraData::GetFPNCalData()
{ 
	vector<cv::Mat> fpnCal;

	for (int i = 0; i < _imgFPNCal.size(); i++)
	{
		fpnCal.push_back(_imgFPNCal[i].clone());
	}

	return fpnCal;
}

void CameraData::SetFPNCalData(vector<cv::Mat> fpnCal)
{
	if (fpnCal.size() == _imgFPNCal.size())
	{
		for (int i = 0; i < _imgFPNCal.size(); i++)
		{
			fpnCal[i].copyTo(_imgFPNCal[i]);
		}
	}
}

bool CameraData::ChangeGainIndex(int inc)
{
	bool changed = false;
	if ((inc > 0) && (_gainAutoCurrentIndex < (_gainAutoValues.size() - 1)))
	{
		_gainAutoCurrentIndex++;
		changed = true;
	}
	else if ((inc < 0) && (_gainAutoCurrentIndex > 0))
	{
		_gainAutoCurrentIndex--;
		changed = true;
	}

	if (changed)
	{
		LOG4CXX_INFO(logger, "ChangeGainIndex for camera " << CameraNames[(int)_cameraType] << " to index " << _gainAutoCurrentIndex);
		SetGain(_gainAutoValues[_gainAutoCurrentIndex]);
	}

	return changed;
}

int CameraData::FindClosestGainIndex(int val)
{
	int closestIndex = 0;
	int maxDiff = INT_MAX;
	for (int i = 0; i < _gainAutoValues.size(); i++)
	{
		int diff = abs(_gainAutoValues[i] - val);
		if (diff < maxDiff)
		{
			closestIndex = i;
			maxDiff = diff;

		}
	}

	return closestIndex;
}

int CameraData::GetGainIndexValue(int index)
{
	int gain = 0;
	if ((index >= 0) && (index < _gainAutoValues.size()))
	{
		gain = _gainAutoValues[index];
	}

	return gain;
}

int CameraData::GetGainMaxIndexValue()
{
	int maxValue = 0;
	for (int i = 0; i < _gainAutoValues.size(); i++)
	{
		maxValue = max(maxValue, _gainAutoValues[i]);
	}

	return maxValue;
}