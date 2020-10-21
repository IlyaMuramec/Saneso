#pragma once

#include "opencv2/core/core.hpp"
#include "FramesPerSecond.h"
#include "LaplacianBlending.h"
#include "ProcTimes.h"
#include "StatusClasses.h"
#include <log4cxx\logger.h>

#define PI 3.1415

//class CSystemManager;

enum ImageStitchingMode
{
	eNormal,
	//eNoStitching,
	eClover,
	eFrontOnly,
	eLastMode
};

class CameraMosaic
{
public: 
	cv::Mat _image;
	cv::Mat _mask;
	cv::Rect _boundingRect;
};

class MosaicData
{
public:
	MosaicData(ProcTimes *pProcTimes);
	~MosaicData(void);
	
	cv::Size GetSize()			{ return _size; }
	cv::Point GetCenter()		{ return _center; }
	void SetSize(cv::Size size);

	void CreateMosaic(cv::Mat &cvMosaicImg);

	bool IsBlendingEnabled()			{ return _enableBlending; }
	void EnableBlending(bool enable)	{ _enableBlending = enable; }
	int GetBlendWidth()					{ return _blendWidth; }

	void DrawCrossHairs(cv::Mat &mosaic);
	void DrawCrossHairsBlack(cv::Mat &mosaic);
	void ApplyMask(cv::Mat &mosaic);

	double GetFrameRate()			{ return _framesPerSecond.CalcFPS(); }

	void SetCrossHairRadius(double radius) { _crossHairRadius = radius; }
	void SetOutputFolder(std::string folder)	{ _saveFolder = folder; }

	void SetCameraMosaic(CameraType camera, cv::Mat  image, cv::Mat mask, cv::Rect rect, cv::Rect borderRect);
	void UpdateMosaic(CameraType camera, Mat &mosaic);

	cv::Rect GetFrontBorderRect() { return _borderRect; }
	cv::Scalar GetFrontBorderAverageValue(CameraType camera) { return _frontBorderAverageValues[camera]; }
	cv::Scalar GetBorderAverage(CameraType side, Mat image);

	double GetSideCameraScale()		{ return _sideCameraScale; }
	double GetSideCameraRadius()	{ return _sideCameraRadius; }
	double GetSideCameraOffset()	{ return _sideCameraOffset; }

	void SetImageStitchingMode(ImageStitchingMode mode)	{ m_eImageStitchingMode = mode; }
	ImageStitchingMode GetImageStitchingMode()			{ return m_eImageStitchingMode; }

	void EnableApplyMask(bool enable)	{ m_bApplyMaskEnabled = enable; }
	BOOL IsApplyMaskEnabled()			{ return m_bApplyMaskEnabled; }

	void EnableMarkerDislay(bool enable) { m_bMarkerDisplayEnabled = enable; }
	BOOL IsMarkerDisplayEnabled()		{ return m_bMarkerDisplayEnabled; }

private:
	static log4cxx::LoggerPtr logger;

	cv::Size _size;
	cv::Point _center;
	double _ring30Radius;
	double _ring0Radius;
	double _crossHairRadius;
	bool _enableBlending;
	int _blendWidth;
	cv::Mat _mask;
	cv::Mat _tempMosiac;
	std::string _saveFolder;
	CFramesPerSecond _framesPerSecond;
	ImageStitchingMode m_eImageStitchingMode;
	bool m_bMarkerDisplayEnabled;
	bool m_bApplyMaskEnabled;

	//Clover settings
	double _sideCameraScale;
	double _sideCameraRadius;
	double _sideCameraOffset;

	LaplacianBlending _laplacianBlending;
	std::vector<CameraMosaic> _cameraMosaics;

	std::vector<cv::Scalar> _frontBorderAverageValues;
	cv::Rect _borderRect;
};

