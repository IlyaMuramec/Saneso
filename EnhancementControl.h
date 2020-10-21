#pragma once

#include <vector>
#include <opencv2\core\core.hpp>
#include "json\json.h"

#define MAX_NUM_ENHANCEMENT_PRESETS 8

class CEnhancementSettings
{
public:
	CEnhancementSettings();

	void Load(Json::Value value);
	void Save(Json::Value &value);

	int GetContrastEnhancement() { return m_nContrastEnhancement; }
	void SetContrastEnhancement(int value) { m_nContrastEnhancement = value; }

	int GetSharpnessEnhancement() { return m_nSharpnessEnhancement; }
	void SetSharpnessEnhancement(int value) { m_nSharpnessEnhancement = value; }

	bool IsContrastEnhancementEnabled() { return m_bContrastEnhancementEnabled; }
	void EnableContrastEnhancement(bool enabled) { m_bContrastEnhancementEnabled = enabled; }

	bool IsSharpnessEnhancementEnabled() { return m_bSharpnessEnhancementEnabled; }
	void EnableSharpnessEnhancement(bool enabled) { m_bSharpnessEnhancementEnabled = enabled; }

	bool IsTemporalAveragingEnabled() { return m_bTemporalAveragingEnabled; }
	void EnableTemporalAveraging(bool enabled) { m_bTemporalAveragingEnabled = enabled; }

	int GetTemporalAveraging()		{ return m_nTemporalAveraging; }
	void SetTemporalAveraging(int value)	{ m_nTemporalAveraging = value; }

	bool GetTemporalMedianAveraging() { return m_bTemporalMedianAveraging; }
	void SetTemporalMedianAveraging(bool enabled) { m_bTemporalMedianAveraging = enabled; }

	bool IsSpatialAveragingEnabled() { return m_bSpatialAveragingEnabled; }
	void EnableSpatialAveraging(bool enabled) { m_bSpatialAveragingEnabled = enabled; }

	int GetSpatialAveraging()		{ return m_nSpatialAveraging; }
	void SetSpatialAveraging(int value)	{ m_nSpatialAveraging = value; }

	bool GetSpatialMedianAveraging() { return m_bSpatialMedianAveraging; }
	void SetSpatialMedianAveraging(bool enabled) { m_bSpatialMedianAveraging = enabled; }

	bool IsBrightnessEnabled()	{ return m_bBrightnessEnabled; }
	void EnableBrightness(bool enabled) { m_bBrightnessEnabled = enabled; }

	float GetBrightness()	{ return m_fBrightness; }
	void SetBrightness(float value) { m_fBrightness = value; }

	bool IsGammaEnabled()	{ return m_bGammaEnabled; }
	void EnableGamma(bool enabled) { m_bGammaEnabled = enabled; }
	
	float GetGamma()	{ return m_fGamma; }
	void SetGamma(float value) { m_fGamma = value; }

	bool IsGammaSideEnabled()	{ return m_bGammaSideEnabled; }
	void EnableGammaSide(bool enabled) { m_bGammaSideEnabled = enabled; }

	float GetGammaSide()	{ return m_fGammaSide; }
	void SetGammaSide(float value) { m_fGammaSide = value; }

	bool IsFrontScaleEnabled()	{ return m_bFrontScaleEnabled; }
	void EnableFrontScale(bool enabled) { m_bFrontScaleEnabled = enabled; }

	float GetFrontScaleSlope()	{ return m_fFrontScaleSlope; }
	void SetFrontScaleSlope(float value) { m_fFrontScaleSlope = value; }

	bool IsSideScaleEnabled()	{ return m_bSideScaleEnabled; }
	void EnableSideScale(bool enabled) { m_bSideScaleEnabled = enabled; }

	float GetSideScaleSlope()	{ return m_fSideScaleSlope; }
	void SetSideScaleSlope(float value) { m_fSideScaleSlope = value; }

	float GetSideScaleOffset()	{ return m_fSideScaleOffset; }
	void SetSideScaleOffset(float value) { m_fSideScaleOffset = value; }

	int GetUpsampleFactor() { return m_nUpsampleFactor; }
	void SetUpsampleFactor(int value) { m_nUpsampleFactor = value; }

private:
	bool m_bContrastEnhancementEnabled;
	bool m_bSharpnessEnhancementEnabled;
	int m_nContrastEnhancement;
	int m_nSharpnessEnhancement;

	bool m_bTemporalAveragingEnabled;
	int m_nTemporalAveraging;
	bool m_bTemporalMedianAveraging;

	bool m_bSpatialAveragingEnabled;
	int m_nSpatialAveraging;
	bool m_bSpatialMedianAveraging;

	bool m_bBrightnessEnabled;
	float m_fBrightness;

	bool m_bGammaEnabled;
	float m_fGamma;

	bool m_bGammaSideEnabled;
	float m_fGammaSide;

	bool m_bFrontScaleEnabled;
	float m_fFrontScaleSlope;

	bool m_bSideScaleEnabled;
	float m_fSideScaleSlope;
	float m_fSideScaleOffset;

	int m_nUpsampleFactor;
};

class CEnhancementControl
{
public:
	CEnhancementControl();
	~CEnhancementControl();

	void Load(std::string folder);
	void Save(std::string folder);
	
	int GetCurrentPreset() { return m_nCurrentPreset; }
	void SetCurrentPreset(int preset);

	CEnhancementSettings *GetCurrentSettings();
	bool ContainsPreset(int index);
	
	void EnhanceImage(cv::Mat input, cv::Mat& output);

	void EnableEnhancement(bool enable);
	bool IsEnhancementEnabled()			{ return m_bEnhancementEnabled; }

private:
	int m_nCurrentPreset;
	std::map<int, CEnhancementSettings*> m_SettingPresets;
	bool m_bEnhancementEnabled;

	void ClearSettings();
};

