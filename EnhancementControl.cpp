#include "stdafx.h"
#include "EnhancementControl.h"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <fstream>

using namespace cv;
using namespace std;

CEnhancementSettings::CEnhancementSettings()
{
	m_bContrastEnhancementEnabled = false;
	m_nContrastEnhancement = 1;

	m_bSharpnessEnhancementEnabled = false;
	m_nSharpnessEnhancement = 1;
	
	m_bTemporalAveragingEnabled = false;
	m_nTemporalAveraging = 2;
	m_bTemporalMedianAveraging = false;
	
	m_bSpatialAveragingEnabled = false;
	m_nSpatialAveraging = 2;
	m_bSpatialMedianAveraging = false;

	m_bGammaEnabled = false;
	m_fGamma = 1.0; 

	m_bGammaSideEnabled = false;
	m_fGammaSide = 1.0;

	m_bBrightnessEnabled = false;
	m_fBrightness = 0.0;

	m_bFrontScaleEnabled = false;
	m_fFrontScaleSlope = 1.0;

	m_bSideScaleEnabled = false;
	m_fSideScaleSlope = 0.5;
	m_fSideScaleOffset = 0.6;

	m_nUpsampleFactor = 1;
}

void CEnhancementSettings::Load(Json::Value value)
{
	m_bContrastEnhancementEnabled = (value.get("ContrastEnhancementEnabled", 0).asInt() == 1);
	m_nContrastEnhancement = value.get("ContrastEnhancementValue", 1).asInt();

	m_bSharpnessEnhancementEnabled = (value.get("SharpnessEnhancementEnabled", 0).asInt() == 1);
	m_nSharpnessEnhancement = value.get("SharpnessEnhancementValue", 1).asInt();

	m_bTemporalAveragingEnabled = (value.get("TemporalAveragingEnabled", 0).asInt() == 1);
	m_nTemporalAveraging = value.get("TemporalAveragingValue", 2).asInt();
	m_bTemporalMedianAveraging = (value.get("TemporalMedianAveraging", 0).asInt() == 1);

	m_bSpatialAveragingEnabled = (value.get("SpatialAveragingEnabled", 0).asInt() == 1);
	m_nSpatialAveraging = value.get("SpatialAveragingValue", 2).asInt();
	m_bSpatialMedianAveraging = (value.get("SpatialMedianAveraging", 0).asInt() == 1);

	m_bGammaEnabled = (value.get("GammaEnabled", 0).asInt() == 1);
	m_fGamma = value.get("Gamma", 1).asFloat();

	m_bGammaSideEnabled = (value.get("GammaSideEnabled", 0).asInt() == 1);
	m_fGammaSide = value.get("GammaSide", 1).asFloat();

	m_bBrightnessEnabled = (value.get("BrightnessEnabled", 0).asInt() == 1);
	m_fBrightness = value.get("Brightness", 0).asFloat();

	m_bFrontScaleEnabled = (value.get("FrontScaleEnabled", 0).asInt() == 1);
	m_fFrontScaleSlope = value.get("FrontScaleSlope", 1.0).asFloat();

	m_bSideScaleEnabled = (value.get("SideScaleEnabled", 0).asInt() == 1);
	m_fSideScaleSlope = value.get("SideScaleSlope", 0.35).asFloat();
	m_fSideScaleOffset = value.get("SideScaleOffset", 1.2).asFloat();

	m_nUpsampleFactor = value.get("UpsampleFactor", 1).asInt();
}

void CEnhancementSettings::Save(Json::Value &value)
{
	value["ContrastEnhancementEnabled"] = (m_bContrastEnhancementEnabled ? 1 : 0);
	value["ContrastEnhancementValue"] = m_nContrastEnhancement;

	value["SharpnessEnhancementEnabled"] = (m_bSharpnessEnhancementEnabled ? 1 : 0);
	value["SharpnessEnhancementValue"] = m_nSharpnessEnhancement;

	value["TemporalAveragingEnabled"] = (m_bTemporalAveragingEnabled ? 1 : 0);
	value["TemporalAveragingValue"] = m_nTemporalAveraging;
	value["TemporalMedianAveraging"] = (m_bTemporalMedianAveraging ? 1 : 0);

	value["SpatialAveragingEnabled"] = (m_bSpatialAveragingEnabled ? 1 : 0);
	value["SpatialAveragingValue"] = m_nSpatialAveraging;
	value["SpatialMedianAveraging"] = (m_bSpatialMedianAveraging ? 1 : 0);

	value["GammaEnabled"] = (m_bGammaEnabled ? 1 : 0);
	value["Gamma"] = m_fGamma;

	value["GammaSideEnabled"] = (m_bGammaEnabled ? 1 : 0);
	value["GammaSide"] = m_fGamma;

	value["BrightnessEnabled"] = (m_bBrightnessEnabled ? 1 : 0);
	value["Brightness"] = m_fBrightness;

	value["FrontScaleEnabled"] = (m_bFrontScaleEnabled ? 1 : 0);
	value["FrontScaleSlope"] = m_fFrontScaleSlope;

	value["SideScaleEnabled"] = (m_bSideScaleEnabled ? 1 : 0);
	value["SideScaleSlope"] = m_fSideScaleSlope;
	value["SideScaleOffset"] = m_fSideScaleOffset;

	value["UpsampleFactor"] = m_nUpsampleFactor;
}

CEnhancementControl::CEnhancementControl()
{
	m_bEnhancementEnabled = true;
	m_nCurrentPreset = 0;
	m_SettingPresets[0] = new CEnhancementSettings(); // all off
}


CEnhancementControl::~CEnhancementControl()
{
	ClearSettings();
}

void CEnhancementControl::ClearSettings()
{
	for (int i = 0; i < m_SettingPresets.size(); i++)
	{
		delete m_SettingPresets[i];
	}
	m_SettingPresets.clear();
}

void CEnhancementControl::SetCurrentPreset(int preset)
{
	 m_nCurrentPreset = preset;
	 m_bEnhancementEnabled = true;
}

void CEnhancementControl::EnableEnhancement(bool enable)
{
	m_bEnhancementEnabled = enable;
}


void CEnhancementControl::Load(string folder)
{
	ClearSettings();
	m_SettingPresets[0] = new CEnhancementSettings(); // all off

	string fName = folder + "\\Enhancement.json";
	
	ifstream file(fName);

	Json::Value root;
	Json::Reader reader;

	bool parsingSuccessful = reader.parse(file, root);
	if (parsingSuccessful)
	{
		m_bEnhancementEnabled = (root.get("EnhancementEnabled", 1).asInt() == 1);
		m_nCurrentPreset = root.get("CurrentPreset", 1).asInt();

		for (int i = 1; i <= MAX_NUM_ENHANCEMENT_PRESETS; i++)
		{
			string preset = std::to_string(i);
			Json::Value const* found = root.find(preset.data(), preset.data() + preset.length());
			if (found)
			{
				m_SettingPresets[i] = new CEnhancementSettings();
				m_SettingPresets[i]->Load(root[preset]);
			}
		}
	}
}

void CEnhancementControl::Save(string folder)
{
	Json::Value root;

	root["EnhancementEnabled"] = (m_bEnhancementEnabled ? 1 : 0);
	root["CurrentPreset"] = (m_nCurrentPreset ? 1 : 0);

	for (int i = 1; i < m_SettingPresets.size(); i++)
	{
		string preset = std::to_string(i);
		m_SettingPresets[i]->Save(root[preset]);
	}

	string fName = folder + "\\Enhancement.json";
	ofstream file(fName);
	Json::StreamWriterBuilder styledWriterBuilder;
	styledWriterBuilder.settings_["precision"] = 5;
	file << Json::writeString(styledWriterBuilder, root);
	file.close();
}

CEnhancementSettings* CEnhancementControl::GetCurrentSettings()
{
	int preset = m_nCurrentPreset;
	if (!m_bEnhancementEnabled || !ContainsPreset(preset))
	{
		preset = 0;
	}
		
	return m_SettingPresets[preset];
}

bool CEnhancementControl::ContainsPreset(int index)
{
	return (m_SettingPresets.find(index) != m_SettingPresets.end());
}

void CEnhancementControl::EnhanceImage(cv::Mat input, cv::Mat& output)
{
	cv::Mat contrastEnhanced;
	
	CEnhancementSettings* pSettings = GetCurrentSettings();

	int nContrastEnhancement = pSettings->GetContrastEnhancement();
	if (pSettings->IsContrastEnhancementEnabled() 
		&& (nContrastEnhancement > 0))
	{
#if 0
		Mat inputFlt;
		input.convertTo(inputFlt, CV_32FC3);
		
		Mat grayImageFlt;
		cvtColor(inputFlt, grayImageFlt, CV_RGB2GRAY);
		Scalar avgVal = mean(grayImageFlt);

		grayImageFlt /= avgVal.val[0];
		pow(grayImageFlt, 0.01 * nContrastEnhancement, grayImageFlt);

		vector<Mat> layers;
		split(inputFlt, layers);
		for (int i = 0; i < layers.size(); i++)
		{
			multiply(layers[i], grayImageFlt, layers[i]);
			layers[i].convertTo(layers[i], CV_8U);
		}
		merge(layers, contrastEnhanced);
#else
		Mat grayImage;
		cvtColor(input, grayImage, CV_RGB2GRAY);
		
		Scalar avgVal = mean(grayImage);
		grayImage -= avgVal.val[0];
		grayImage *= (0.02 * nContrastEnhancement);

		vector<Mat> layers;
		split(input, layers);
		for (int i = 0; i < layers.size(); i++)
		{
			add(layers[i], grayImage, layers[i]);
		}
		merge(layers, contrastEnhanced);
#endif
	
	}
	else
	{
		contrastEnhanced = input.clone();
	}

	int nSharpnessEnhancement = pSettings->GetSharpnessEnhancement();
	if (pSettings->IsSharpnessEnhancementEnabled() 
		&& (nSharpnessEnhancement > 0))
	{
		Mat imgBlur;

		cv::blur(contrastEnhanced, imgBlur, cv::Size(nSharpnessEnhancement, nSharpnessEnhancement));
		//cv::GaussianBlur(colorEnhanced, imgBlur, cv::Size(0, 0), ceil(0.1*m_nContrastEnhancement));
		cv::addWeighted(contrastEnhanced, 2, imgBlur, -1, 0, output);
	}
	else
	{
		output = contrastEnhanced.clone();
	}
}