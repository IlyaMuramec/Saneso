#pragma once

#include "SystemManager.h"


class CSelfCalibrationDlg;
	
class CSelfCalibration
{
public:
	CSelfCalibration(CSystemManager *pSystemManager);
	~CSelfCalibration();

	bool StartCalibration();
	void SetSelfCalibrationDlg(CSelfCalibrationDlg *pDlg);
	void ResetCalibration();
	bool IsCalibrating()	{ return m_bIsCalibrating; }
	SelfCalibrationStatus GetLastSelfCalibration() { return m_LastSelfCalibration; }

private:
	CSystemManager *m_pSystemManager;
	CSelfCalibrationDlg *m_pSelfCalibrationDlg;

	SelfCalibrationSettings m_SelfCalibrationSettings;
	SelfCalibrationStatus m_CurrentSelfCalibration;
	SelfCalibrationStatus m_LastSelfCalibration;

	bool m_bIsCalibrating;

	HANDLE m_hThread;
	static UINT ThreadProc(LPVOID param)
	{
		return ((CSelfCalibration*)param)->SelfCalibration();
	}
	UINT SelfCalibration();
	
	void UpdateDialog(SelfCalibrationStatus *pStatus);

	void VerifyCameras();
	void VerifyCalCup();
	void VerifyLights();
	void FixedPatternNoiseCal();
	void WhiteBalanceCal();
	bool IsOutsideLimits(float value, float limit, float tolerance);
};

