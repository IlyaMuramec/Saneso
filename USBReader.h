#pragma once

#include "StatusClasses.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <vector>
#include <queue>
#include "afxmt.h"
#include <log4cxx\logger.h>
#include "CyAPI.h"
#include <opencv2\core\core.hpp>

class CameraData;

class CUSBReader
{
public:
	enum StreamStatus
	{
		Startup,
		Opened,
		Streaming,
		Recovery,
		Error,
		Closed
	};

	struct CaptureThreadParams
	{
		CUSBReader *pUSBReader;
		int nCameraIndex;
	};

	CUSBReader();
	~CUSBReader();

	int Initialize(std::vector<CameraData*> &cameras, std::string debugImageFolder, std::string simImageFolder);
	void StartCapture();
	void StopCapture();
	void SaveNextTransfer(std::string folder);
	void ResetCameras(unsigned int mask);
	void ResetAllUSBChips();
	
	void SetSimImageFolder(std::string folder)	{ m_sSimImageFolder = folder; }
	std::string GetFWVersion(int camera);
	float GetFWVersionNum(int camera);
	CompStatus GetStatus();
	int GetResetCount() { return m_nResetCount; }
	void IncrementResetCount() { m_nResetCount++; }
	void InhibitRecovery(bool inhibit);
	void EnableFrontSplitImageDetection(bool enable) { m_bFrontSplitImageDetectionEnabled = enable; }

private:
	static log4cxx::LoggerPtr logger;

	static const int QueueSize;
	static const int MAX_QUEUE_SZ;
	static const int TimeOut;
	static const int BYTES_PER_PIXEL;
	static int PPX;

	CCriticalSection m_CriticalSection;
	int m_nResetCount;
	bool m_bInhibitRecovery;
	bool m_bFrontSplitImageDetectionEnabled;

	std::vector<CameraData*> m_vCameras;
	std::vector<cv::Mat>	m_vSimImages;
	std::vector<HANDLE>		m_vCaptureThreads;
	std::vector<HANDLE>		m_vSimThreads;
	std::vector<HANDLE>		m_vSimTimers;
	std::vector<CRITICAL_SECTION> m_vCriticalSections;
	std::string				m_sDebugImageFolder;
	std::string				m_sSimImageFolder;
	std::vector<bool>		m_vSaveNextTransfer;
	std::vector<bool>		m_vResetCameraMask;
	std::vector<bool>		m_vExitFlags;
	std::vector<StreamStatus> m_vStreamStatus;
	std::vector<std::string> m_vFWVersions;
	std::vector<CCyUSBDevice *> m_vUSBDevices;

	static UINT CaptureThreadProc(LPVOID param)
	{
		CaptureThreadParams* pThreadParams = (CaptureThreadParams*)param;
		LOG4CXX_INFO(logger, "CaptureThreadProc " << pThreadParams->nCameraIndex);

		UINT retVal = pThreadParams->pUSBReader->CaptureThread(pThreadParams->nCameraIndex);
		delete param;

		return retVal;
	}
	UINT CaptureThread(int camera);


	static UINT ThreadProc(LPVOID param);
	static VOID CALLBACK SimTimerProc(PVOID param, BOOLEAN TimerOrWaitFired)
	{
		((CaptureThreadParams*)param)->pUSBReader->SimTimer(((CaptureThreadParams*)param)->nCameraIndex);
	}
	VOID SimTimer(int camera);

	
	void SendVendorCommand(CCyControlEndPoint   *CtrlEndPt, UCHAR command);
	bool SetupXferLoop(CCyUSBDevice *USBDevice,
		CCyUSBEndPoint *EndPt, CCyControlEndPoint *CtrlEndPt,
		PUCHAR *buffers, PUCHAR *contexts, OVERLAPPED inOvLap[], int len);
	void AbortXferLoop(CCyUSBEndPoint *EndPt, CCyControlEndPoint   *CtrlEndPt, 
		int pending, long length, PUCHAR *buffers, PUCHAR *contexts, OVERLAPPED inOvLap[]);

	void CloseUSBDevices();
	void OpenUSBDevices();
};

