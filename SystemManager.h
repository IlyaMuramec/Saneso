#pragma once

#include "ProgramSettings.h"
#include "MosaicData.h"
#include "CameraData.h"
#include "LightControl.h"
#include "EnhancementControl.h"
#include "StatusClasses.h"
#include "opencv2\core\core.hpp"

#ifdef USE_USB_CAMERA
#include "USBReader.h"
#else
#include "PentekReader.h"
#endif

#ifdef _DEBUG
// javqui 
#define IGNORE_MICROCONTROLLER
#endif

class CNGEuserDlg;
class CSystemManagerDlg;
class CSplashThread;
class CMicroController;
class CScopeSettings;
class CProcessorBoxSettings;
class CSelfCalibration;
class CSignalOutput;
class CScopeStatus;


enum DisplayMode
{
	eLive,
	eFrozen,
	eRecording,
	ePlayback,
	eDisplayImage
};

enum MicroControllerState
{
	eNoPower,
	eConnected,
	eReflashing,
	eReflashComplete,
	eNoScope
};

class CSystemManager
{
public:
	CSystemManager();
	~CSystemManager();

	void Create(CNGEuserDlg *pDlg);
	void Start(bool bRecovery);
	void StopCapture();
	void SetSystemManagerDlg(CSystemManagerDlg* pDlg) { m_pSystemManagerDlg = pDlg; }

	void ResetUSBChips(CSplashThread *pSplash);

	void SaveConfig(CString file);
	void LoadConfig(CString file);
	string GetDateTime();

	void CaptureImageFile();
	void CaptureVideoFile();

	void Close();

	void SetDisplayMode(DisplayMode mode)					{ m_eCurrentDisplayMode = mode; }
	DisplayMode GetDisplayMode()							{ return m_eCurrentDisplayMode; }

	void SetNewDisplayMode(DisplayMode mode)				{ m_eNewDisplayMode = mode; }
	DisplayMode GetNewDisplayMode()							{ return m_eNewDisplayMode; }

	void SetImageStichingMode(ImageStitchingMode mode);
	void NextImageStitchingMode();
	ImageStitchingMode GetImageStitchingMode();

	void EnableSelectBandImaging(bool enable);
	bool IsSelectBandImagingEnabled()	{ return m_bSelectBandImaging; }

	void EnableApplyMask(bool enable);
	BOOL IsApplyMaskEnabled();

	void EnableMarkerDislay(bool enable);
	BOOL IsMarkerDisplayEnabled();

	void SetModifiedFlag(BOOL modified);
	BOOL GetModifiedFlag()					{ return m_bModifiedFlag; }

	CProgramSettings* GetProgramSettings()	{ return &m_ProgramSettings; }

	size_t GetNumCameras()						{ return m_vCameras.size(); }
	CameraData *GetCameraData(CameraType type);

	MosaicData *GetMosaicData()				{ return m_pMosaicData; }

#ifdef USE_USB_CAMERA
	CUSBReader* GetUSBeader()				{ return &m_USBReader; }
#else
	CPentekReader* GetPentekReader()		{ return &m_PentekReader; }
#endif

	CMicroController* GetMicroController()		{ return m_pMicroController; }
	CLightControl*	GetLightControl()			{ return m_pLightControl; }
	CEnhancementControl* GetEnhancementControl() { return m_pEnhancementControl; }
	CSelfCalibration* GetSelfCalibration()		{ return m_pSelfCalibration; }
	CSignalOutput* GetSignalOutput()			{ return m_pSignalOutput; }
	CScopeStatus* GetScopeStatus()				{ return m_pScopeStatus; }

	void DumpProcTimes(string folder);

	void UpdateAllCameraModels();

	void LoadDefaultSettings();
	void LoadSettings(string folder);

	CompStatus GetCameraStatus(double fps);
	CompStatus GetMosaicStatus(double fps);
	CompStatus GetControllerStatus(double heartbeat);

	void CalibrateCamerasWhite(unsigned int mask);
	void CalibrateCamerasFPN(unsigned int mask, CSplashThread *pSplash);
	void ResetCameras(unsigned int mask);
	void InitializeCameras(unsigned int mask);

	static void MicroControllerConnectedCallback(void* pObject);

	bool IsScopeEEPromCurrent();
	void ReadEEPromSettings();
	int WriteCurrentToEEProm();

	SystemStatus GetSystemStatus() { return m_SystemStatus; }
	std::vector<double> GetMeasurements(Measurement type);

	void SetMicroControllerReflashing(bool val);
	void UpdateScopeStatus(BYTE status);

	CProcessorBoxSettings *GetProcessorBoxSettings() { return m_pProcessorEEPromSettings; }

private:
	static log4cxx::LoggerPtr logger;

	CCriticalSection m_CritSection;

	CNGEuserDlg *m_pMainDlg;
	CSystemManagerDlg *m_pSystemManagerDlg;

	CProgramSettings	m_ProgramSettings;

#ifdef USE_USB_CAMERA
	CUSBReader			m_USBReader;
#else
	CPentekReader		m_PentekReader;
#endif

	CMicroController    *m_pMicroController;
	CLightControl       *m_pLightControl;
	CEnhancementControl *m_pEnhancementControl;
	CSelfCalibration	*m_pSelfCalibration;
	CSignalOutput		*m_pSignalOutput;
	CScopeStatus		*m_pScopeStatus;
	BOOL				m_bModifiedFlag;
	BOOL				m_bImageDebuggingEnabled;
	BOOL				m_bStreamUpdateEnabled;
	DisplayMode			m_eNewDisplayMode;
	DisplayMode			m_eCurrentDisplayMode;

	CString m_sRunDirectory;
	CString m_sTempFolder;
	CString m_sConfigFile;

	ProcTimes m_ProcTimes;
	MosaicData *m_pMosaicData;
	std::vector<CameraData*> m_vCameras;
	SystemStatus m_SystemStatus;

	bool m_bExiting;
	HANDLE m_hMonitorThread;
	static UINT ThreadProc(LPVOID param)
	{
		return ((CSystemManager*)param)->MonitorLoop();
	}
	UINT MonitorLoop();
	void StartMonitor();
	void StopMonitor();

	HANDLE m_hMicroControllerReconnectThread;
	void StartMicroControllerReconnectThread();
	UINT MicroControllerReconnectThread();
	static UINT MicroControllerReconnectThreadProc(LPVOID param)
	{
		return ((CSystemManager*)param)->MicroControllerReconnectThread();
	}

	HANDLE m_hScopeReconnectThread;
	void StartScopeReconnectThread();
	UINT ScopeReconnectThread();
	static UINT ScopeReconnectThreadProc(LPVOID param)
	{
		return ((CSystemManager*)param)->ScopeReconnectThread();
	}

	//void MicroControllerConnected();
	CScopeSettings* ReadScopeSettings();
	int WriteScopeSettings(CScopeSettings *pSettings);
	VOID CALLBACK ReflashComplete(HWND hwnd, UINT  uMsg, UINT_PTR idEvent, DWORD dwTime);

	CScopeSettings *m_pScopeEEPromSettings;
	CScopeSettings *m_pScopeCurrentSettings;

	CProcessorBoxSettings* ReadProcessorBoxSettings();
	int WriteProcessorBoxSettings(CProcessorBoxSettings *pSettings);
	CProcessorBoxSettings *m_pProcessorEEPromSettings;

	bool m_bSelectBandImaging;
	MicroControllerState m_MicroControllerState;
};

