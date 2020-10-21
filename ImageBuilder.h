#pragma once

#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <log4cxx\logger.h>

class CameraData;

class CImageBuilder
{
	enum FrameState
	{
		eUnknown,
		eOutOfFrame,
		eInSkipFrame,
		eOutOfLine,
		eInLine,
	};

public:
	CImageBuilder(CameraData *camera, std::string debugImageFolder);
	~CImageBuilder();
	
	void Reset();
	void AddPixel(unsigned short pixelVal, bool vsync, bool hsync, bool filterFrames);
	void SaveNextTransfer()	{ m_bSaveNextTransfer = true; }

private:
	static log4cxx::LoggerPtr logger;

	void SendImage();

	CameraData *m_pCamera;
	std::string m_sDebugImageFolder;
	cv::Mat m_BayerImage;
	int m_nRowIndex;
	int m_nColIndex;
	int m_nHSyncCount;
	int m_nVSyncCount;
	int m_nSensorWidth;
	int m_nSensorHeight;
	bool m_bFrameValid;
	bool m_bLineValid;
	int m_nSampleCount;
	BYTE *m_pPxlRow;
	unsigned long m_nCurrentFrame;
	FrameState m_FrameState;
	bool m_bSaveNextTransfer;
};

