#pragma once

#include "CameraData.h"
#include "LightCalSettings.h"
#include <map>
#include <queue>

class CMicroController;

#define NUM_COLORS 8
#define SHADOW_CAST_DELAY_MS_MIN 1
#define SHADOW_CAST_DELAY_MS_MAX 10000

class AutoLightData
{
public:
	double Seconds;
	bool SatOrAverage;
	double SetPoint;
	double ValueR;
	double ValueG;
	double ValueB;
	double P;
	double I;
	double D;
	double Error;
	double Adjustment;
	double PrevLight;
	double NewLight;
	int PrevGain;
	int NewGainRaw;
	int NewGainIndex;
	double DeadBand;
	int GainInc;

	static string GetHeader();
	string GetData();
};

enum LightSide
{
	LEFT,
	RIGHT
};

enum AutoMode
{
	LIGHTS,
	CAMERA_GAIN,
	BOTH,
	LAST_AUTO_MODE
};

typedef void(*UpdateLightCallbackType)(void *, CameraType camera, LightSide side, float val);
typedef void(*SelectBandImagingCallbackType)(void *, bool enable);


class CLightControl
{
public:
	CLightControl(CMicroController *pMicroController);
	~CLightControl();

	void ReloadSettings(string folder);
	void SaveSettings(string folder);

	CLightCalSettings* GetLightCalSettings() { return &m_LightCalSettings; }

	void SetUpdateLightCallBack(UpdateLightCallbackType callback, void *pObject);
	void AddSelectBandImagingCallBack(SelectBandImagingCallbackType callback, void *pObject);
	void StopAuto();

	COLORREF GetColorValue(int index);
	COLORREF GetFrontSelectBandColorValue()	{ return m_ColorSelectBandFront; }
	COLORREF GetSideSelectBandColorValue() { return m_ColorSelectBandSide; }
	
	int GetCurrentFrontColor()				{ return m_nCurrentFrontColor; }
	void SetCurrentFrontColor(int index, bool force = false);
	int GetCurrentSideColor()				{ return m_nCurrentSideColor; }
	void SetCurrentSideColor(int index, bool force = false);
	int GetNumColors()	{ return m_ColorValues.size(); }

	float GetCameraValue(CameraType camera, LightSide side);
	void SetCameraValue(CameraType camera, LightSide side, float value, bool bUpdateMC);

	float GetFrontLightsLevel()				{ return m_fFrontLightsLevel; }
	void SetFrontLightsLevel(float fLevel, bool bUpdateMC);
	float GetSideLightsLevel()				{ return m_fSideLightsLevel; }
	void SetSideLightsLevel(float fLevel, bool bUpdateMC);

	bool GetFrontLightsOn()				{ return m_bFrontLightsOn; }
	void SetFrontLightsOn(bool bOn, bool bUpdateMC);
	bool GetSideLightsOn()				{ return m_bSideLightsOn; }
	void SetSideLightsOn(bool bOn, bool bUpdateMC);

	bool GetFrontLightsAuto()				{ return m_bFrontLightsAuto; }
	void SetFrontLightsAuto(bool bAuto, bool bUpdateMC);
	bool GetSideLightsAuto()				{ return m_bSideLightsAuto; }
	void SetSideLightsAuto(bool bAuto, bool bUpdateMC);

	bool GetShadowCastOn()					{ return m_bShadowCastOn; }
	void SetShadowCastOn(bool bOn);
	bool GetShadowCastGroup1(CameraType camera);
	bool GetShadowCastGroup2(CameraType camera);
	void UpdateShadowGroup1(CameraType camera, bool bOn);
	void UpdateShadowGroup2(CameraType camera, bool bOn);
	void SetShadowCastGroups(std::set<CameraType> &group1, std::set<CameraType> &group2);
	int GetShadowCastDelayMs()				{ return m_nShadowCastDelayMs;  }
	void SetShadowCastDelayMs(int delayMs);
	void UpdateShadowCast();
	
	void ToggleDemoMode();

	bool IsSelectBandImagingEnabled()	{ return m_bSelectBandImaging; }
	void EnableSelectBandImaging(bool enable);

	void SetCustomColor(COLORREF color);

	void SetAutoSatOrAvg(bool val)	{ m_bAutoSatOrAvg = val; }
	bool GetAutoSatOrAvg()			{ return m_bAutoSatOrAvg; }

	void SetAutoSatSetpoint(float val)	{ m_fAutoSatSetpoint = val; }
	float GetAutoSatSetpoint()			{ return m_fAutoSatSetpoint; }
	
	void SetAutoAvgSetpoint(float val)	{ m_fAutoAvgSetpoint = val; }
	float GetAutoAvgSetpoint()			{ return m_fAutoAvgSetpoint; }

	void SetAutoSatGainP(float val)	{ m_fAutoSatGainP = val; }
	float GetAutoSatGainP()			{ return m_fAutoSatGainP; }

	void SetAutoAvgGainP(float val)	{ m_fAutoAvgGainP = val; }
	float GetAutoAvgGainP()			{ return m_fAutoAvgGainP; }

	void SetAutoSatGainI(float val)	{ m_fAutoSatGainI = val; }
	float GetAutoSatGainI()			{ return m_fAutoSatGainI; }

	void SetAutoAvgGainI(float val)	{ m_fAutoAvgGainI = val; }
	float GetAutoAvgGainI()			{ return m_fAutoAvgGainI; }

	void SetAutoSatGainD(float val)	{ m_fAutoSatGainD = val; }
	float GetAutoSatGainD()			{ return m_fAutoSatGainD; }

	void SetAutoAvgGainD(float val)	{ m_fAutoAvgGainD = val; }
	float GetAutoAvgGainD()			{ return m_fAutoAvgGainD; }

	void SetAutoMode(AutoMode val) { m_AutoMode = val; }
	AutoMode GetAutoMode()		{ return m_AutoMode; }

	void MicroControllerConnected();

	void SaveAutoLightData(string folder);

	bool IsSingleColor(COLORREF color);

	void UpdateLEDMUXPeriod(unsigned int period, bool bUpdateMC);
	
private:
	static log4cxx::LoggerPtr logger;
	CMicroController *m_pMicroController;
	CLightCalSettings m_LightCalSettings;
	std::map<int, COLORREF> m_ColorValues;
	int m_nCurrentFrontColor;
	int m_nCurrentSideColor;

	void *m_pUpdateCallbackObject;
	UpdateLightCallbackType m_UpdateLightCallback;

	std::map<SelectBandImagingCallbackType, void *> m_SelectBandImagingCallbacks;

	std::vector<int> m_FrontChannels;
	std::vector<int> m_SideChannels;
	std::vector<std::vector<float>> m_CameraLightValues;
	std::vector<double> m_CameraPrevSat;
	std::vector<double> m_CameraIntegralSat;
	std::vector<double> m_CameraPrevAvg;
	std::vector<double> m_CameraIntegralAvg;
	bool m_bFrontLightsOn;
	bool m_bSideLightsOn;
	bool m_bFrontLightsAuto;
	bool m_bSideLightsAuto;

	float m_fFrontLightsLevel;
	float m_fSideLightsLevel;

	bool m_bSelectBandImaging;
	COLORREF m_ColorSelectBandFront;
	COLORREF m_ColorSelectBandSide;

	bool m_bCustomColorEnabled;
	COLORREF m_CustomColor;

	bool  m_bShadowCastOn;
	int  m_nShadowCastDelayMs;
	std::set<CameraType> m_ShadowCastGroup1;
	std::set<CameraType> m_ShadowCastGroup2;

	void UpdateChannelValue(int channel, float value, bool bUpdateMC);
	void UpdateChannelValues(std::map<int, float> values, bool bUpdateMC);

	void StopAutoFrontThread();
	static UINT AutoFrontThreadProc(LPVOID param)
	{
		return ((CLightControl*)param)->AutoFrontLoop();
	}
	UINT AutoFrontLoop();
	HANDLE m_hAutoFrontThread;
	bool m_bExitAutoFrontThread;

	void StopAutoSideThread();
	static UINT AutoSideThreadProc(LPVOID param)
	{
		return ((CLightControl*)param)->AutoSideLoop();
	}
	UINT AutoSideLoop();
	HANDLE m_hAutoSideThread;
	bool m_bExitAutoSideThread;

	void ApplyAutoCorrection(CameraType camera);
	int LightChannelLookup(CameraType camera, LightSide side, Color color);

	void StopDemoThread();
	static UINT DemoThreadProc(LPVOID param)
	{
		return ((CLightControl*)param)->DemoLoop();
	}
	UINT DemoLoop();
	void DemoLEDValue(int camera, int side, int color, float value, bool bUpdateMC);
	HANDLE m_hDemoThread;
	bool m_bExitDemoThread;


	float m_fControlGamma;

	bool m_bAutoSatOrAvg;
	AutoMode m_AutoMode;
	float m_fAutoSatSetpoint;
	float m_fAutoAvgSetpoint;
	float m_fAutoSatGainP;
	float m_fAutoAvgGainP;
	float m_fAutoSatGainI;
	float m_fAutoAvgGainI;
	float m_fAutoSatGainD;
	float m_fAutoAvgGainD;
	float m_fAutoPeriodMs;

	vector<clock_t> _autoStartTimes;
	vector<std::queue<AutoLightData>> _autoLightData;
	vector<int> _gainLookup;
};

