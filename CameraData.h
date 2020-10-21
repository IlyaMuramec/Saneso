#pragma once

#include "CameraSettings.h"
#include "MosaicData.h"
#include "EnhancementControl.h"
#include "CThinPlateSpline.h"
#include "FramesPerSecond.h"
#include "opencv2/core/core.hpp"
#include "afxmt.h"
#include "MicroController.h"
#include <log4cxx\logger.h>

#define USE_USB_CAMERA 
#define MAX_RAW_VAL 65536

//#define IMAGE_HEIGHT 480
//#define IMAGE_WIDTH 640

enum SimType
{
	eNone,
	eBayer,
	eRaw
};

enum CameraState
{
	eConstructed,
	eStart,
	eBlank,
	eInit,
	eRunning,
	eNotPresent
};


class CameraData
{
public:
	//CameraData(CameraType type, cv::Point2f center, cv::Size2f size, MosaicData *pMosaicData, ProcTimes *pProcTimes);
	CameraData(CameraType type, MosaicData *pMosaicData, CEnhancementControl *pEnhancement, ProcTimes *pProcTimes);
	~CameraData(void);

	void StartCamera(bool force);
	CameraType GetCameraType() { return _cameraType; }
	string GetName();
	void UpdateImagePoint(int index, Point point);
	void UpdateMosaicPoint(int index, Point point);
	void MoveTiePoint(cv::Point2f before, cv::Point2f after, cv::Size mosaicSize);

	void Load(std::string folder);
	void Save(std::string folder);
	void RefreshCameraSettings();
	void RefreshSensorSettings();

	void SetOutputFolder(std::string folder)	{ _saveFolder = folder; }

	void LoadRawImage(std::string filename);
	void LoadBayerImage(std::string filename);

	
	void UpdateMosaic(Mat &mosaic, bool blend);

	void DrawWarpedImages();

	void StartWarpThread();
	void StopWarpThread();

#if 0
	void WarpEdges(std::string filename, cv::Mat &mosaic);
	void FindEdgeVectors(cv::Mat &mosaic, cv::Mat &mosaicMask, cv::Mat &imgWarpCropped);
	void GetTopEdgeVectors(std::vector<cv::Point2f> &origins, std::vector<cv::Point2f> &directions);
	void GetBottomEdgeVectors(std::vector<cv::Point2f> &origins, std::vector<cv::Point2f> &directions);
	cv::Point2f GetCartPointFromLogPolar(cv::Point2f point);
	
	double GetTopAngle();
	double GetBottomAngle()
#endif
		
	void ResetTempImages();
	void UpdateCameraView(Mat *pMosaicInput = NULL, Mat *pMaskInput = NULL, bool pickPoints = true);

	void SetBayerImage(cv::Mat img, bool bSaveData, std::string sDebugFolder);
	cv::Mat GetCameraImage();
	CCameraSettings* GetCameraSettings() { return &_cameraSettings; }

	Mat GetCameraAlignmentImage(double scale, int pointIndex);
	void UpdateMosaicAlignmentImage(cv::Mat &mosaicOverlay, int pointIndex);


	void DebayerImage();

	HANDLE GetCaptureEvent() { return _hCaptureEvent; }
	HANDLE GetUpdateEvent() { return _hUpdateEvent; }

	CameraState GetCameraState() { return _cameraState;  }

	void GetLastChannelValues(double &red, double &green, double &blue);
	void GetLastSaturationValues(double &red, double &green, double &blue);

	void SetImageStitchingMode(ImageStitchingMode mode)	{ _eImageStitchingMode = mode; }
	ImageStitchingMode GetImageStitchingMode()			{ return _eImageStitchingMode; }

	double GetFrameRate()			{ return _framesPerSecond.CalcFPS(); }

	void GetCameraMosaic(Mat &imageMosaic, Mat &maskMosaic) { imageMosaic = _imgMosaic; maskMosaic = _maskMosaic; }

	void UpdateThinPlateSpline();

	int CalibrateWhiteBalance(double calRedComp, double calGreenComp, double calBlueComp);
	void CalibrateFPN();
	void AbortFPNCal();
	bool IsFPNCalActive() { return _bFPNCalActive; }
	bool IsImageOutOfRange() { return _imageOutOfRange; }
	void ResetImageOutOfRange();

	int InitCameraChip(bool force);
	void MicroControllerConnected(CMicroController *pMicroController);

	int GetUSBCameraNum()	{ return _cameraSettings.GetUSBCameraNum(); }
	bool IsCameraSimulated()	{ return _cameraSettings.IsCameraSimulated(); }

	void SetCameraChip(CameraChip chip);
	CameraChip GetCameraChip()			{ return _cameraSettings.GetCameraChip(); }

	void SetExposure(int val);
	int GetExposure()		{ return _cameraSettings.GetExposure(); }

	void SetGain(int val);
	int GetGain()			{ return _cameraSettings.GetGain(); }

	void EnableAutoExposure(bool enable);
	bool IsAutoExposureEnabled()		{ return _cameraSettings.IsAutoExposureEnabled(); }

	void EnableAutoGain(bool enable);
	bool IsAutoGainEnabled()		{ return _cameraSettings.IsAutoGainEnabled(); }

	void SetColorGains(float red, float green, float blue);
	void GetColorGains(float &red, float &green, float &blue) { _cameraSettings.GetColorGains(red, green, blue); }

	int GetSensorHeight()	{ return _cameraSettings.GetSensorSize().height; }
	int GetSensorWidth()	{ return _cameraSettings.GetSensorSize().width; }

	void SetBayerType(int type)		{ _cameraSettings.SetBayerType(type); }
	int GetBayerType()				{ return _cameraSettings.GetBayerType(); }

	void UpdateLookup(float brightness, float gamma);
	float GetGamma()				{ return _fGamma; }

	void UpdateFrontScaleImage(float slope);
	void UpdateScaleImage(float slope, float offset);
	float GetScaleSlope()			{ return _fScaleSlope; }
	float GetScaleOffset()			{ return _fScaleOffset; }

	void SetMeasurementType(Measurement type);
	double GetMeasurement();
	void DoMeasurement(Mat &_imgMosaic, Mat &_maskMosaic);

	double GetMaxFPNValue();
	vector<cv::Mat> GetFPNCalData();
	void SetFPNCalData(vector<cv::Mat> fpnCal);

	bool ChangeGainIndex(int inc);
	int FindClosestGainIndex(int gain);
	int GetGainIndexValue(int index);
	int GetGainMaxIndexValue();

	bool IsIgnoringFrames()		{ return (_ignoreFrameCount > 0); }

	void SaveDebugImages(string sDebugFolder);

private:
	static log4cxx::LoggerPtr logger;
	CCriticalSection _criticalSectionImage;
	CCriticalSection _criticalSectionMosaic;
	HANDLE _hCaptureEvent;
	HANDLE _hWarpEvent;
	HANDLE _hUpdateEvent;
	CameraType _cameraType;
	CameraState _cameraState;
	CThinPlateSpline _tps;
	CFramesPerSecond _framesPerSecond;
	std::string _saveFolder;
	MosaicData *_pMosaicData;
	CEnhancementControl *_pEnhancementControl;
	ProcTimes *_pProcTimes;
	CMicroController *_pMicroController;
	LaplacianBlending _laplacianBlending;
	CCameraSettings _cameraSettings;

	// Input data
	cv::Mat _imgRaw;
	cv::Mat _imgSensorRGB;
	cv::Mat _imgSensorRGB2;
	cv::Mat _imgBayer;
	cv::Mat _imgTempImage;
	cv::Mat _imgTempMask;
	cv::Mat _maskImageWarp;
	cv::Mat _imgMaskFromMosaic;

	// Mosaic data
	cv::Mat _imgMosaic;
	cv::Mat _maskMosaic;

	cv::Rect _rectImageWarp;
	cv::Rect _rectImageBorder;
	
	float _logPolarM;
	ImageStitchingMode _eImageStitchingMode;
	SimType _bSimType;
	std::vector<cv::Point2f> _topPoints;
	std::vector<cv::Point2f> _topVectors;
	std::vector<cv::Point2f> _bottomPoints;
	std::vector<cv::Point2f> _bottomVectors;
	std::deque<cv::Mat> _temporalAverageQueue;
	std::deque<cv::Mat> _temporalAverageScratch;
	int _temporalAverageQueueSize;
	cv::Mat _temporalAverageSum;
	bool _imageOutOfRange;
	int _imageOutOfRangeIgnoreCount;

	double _averageRed;
	double _averageBlue;
	double _averageGreen;
	double _saturationRed;
	double _saturationBlue;
	double _saturationGreen;

	LONGLONG _perfFreq;
	LONGLONG _lastFrameTime;
	double _frameRate;
	LONGLONG _frameCount;
	int _ignoreFrameCount;
	double _upsampleFactor;

	double GetAngle();
	void GetMosaicMask(cv::Size mosaicSize, cv::Mat &mask);
	void GetCameraMask(cv::Size mosaicSize, cv::Mat &mask);
	void WhiteBalance(const cv::Mat imgUnbalanced, cv::Mat& imgLarge, cv::Mat& mask);
	void LaplacianBlend(cv::Mat& mosaic, const cv::Mat& image, const cv::Mat& mask); 
#if 0
	void FindEdgeVectors(cv::Mat &dstEdges, cv::Rect roi, std::vector<cv::Point2f> &points, std::vector<cv::Point2f> &vectors);
	cv::Point2f GetVector(cv::RotatedRect rotatedRect);
#endif
	void DoWarp(cv::Mat &imgLarge, cv::Mat &imgMaskLarge, cv::Mat &imgWarp, cv::Mat &imgWarpMask);
	void WarpImage(cv::Mat &imgMosaic, cv::Mat &maskMosaic, bool bPickPoints);
	void DrawUnstichedImage(cv::Mat &imgMosaic, cv::Mat &maskMosaic, float ratioFront, float ratioSide);
	void DrawCloverImage(cv::Mat &imgMosaic, cv::Mat &maskMosaic);
	Rect GetCameraROI();
	double CalculateSaturation(Mat& image, Mat& mask);
	void FindCrossHairRadius();
	double median(cv::Mat channel);
	void ApplyLookup(const Mat& src, Mat& dst);
	void UpdateDynamicScaleImage(Mat image, float offset);
	void NextFPNGain();
	void CompleteFPNCal();
	void ClearImage();
	
	bool _bExiting;
	HANDLE _hThread;
	static UINT ThreadProc(LPVOID param)
	{
		return ((CameraData*)param)->WarpLoop();
	}
	UINT WarpLoop();

	// Fixed pattern noise
	vector<cv::Mat> _imgFPNCal;
	cv::Mat _imgFPNAvg;
	bool _bFPNCalActive;
	bool _bAbortFPNCal;
	int _nFPNAvgCount;
	int _nExposureBeforeFPNCal;
	bool _bAutoExposureBeforeFPNCal;
	int _nGainBeforeFPNCal;
	bool _bAutoGainBeforeFPNCal;
	vector<cv::Mat> _imgFPNCalBeforeFPNCal;
	float _fBrightness;
	float _fGamma;
	byte _pGammaLookup[MAX_RAW_VAL];

	cv::Mat _imgScale;
	cv::Mat _imgScaleDynamic;
	float _fScaleSlope;
	float _fScaleOffset;

	vector<float> _scaleLookup;

	Measurement _eMeasurementType;
	Measurement _eMeasurementTypeNew;
	double _dMeasurementValue;
	double _dMeasurementCount;

	vector<int> _gainAutoValues;
	int _gainAutoCurrentIndex;
};

