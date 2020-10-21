#include "stdafx.h"
#include "LightCalSettings.h"


CLightCalSettings::CLightCalSettings()
{
	_fFrontGainRed = 0.05f;
	_fFrontGainGreen = 1.0f;
	_fFrontGainBlue = 1.0f;
	_fSideGainRed = 1.0f;
	_fSideGainGreen = 0.05f;
	_fSideGainBlue = 0.05f;
	_uMUXRate = 41152;
}

CLightCalSettings::~CLightCalSettings()
{
}

void CLightCalSettings::Save(Json::Value &root)
{
	root["FrontGainRed"] = _fFrontGainRed;
	root["FrontGainGreen"] = _fFrontGainGreen;
	root["FrontGainBlue"] = _fFrontGainBlue;
	
	root["SideGainRed"] = _fSideGainRed;
	root["SideGainGreen"] = _fSideGainGreen;
	root["SideGainBlue"] = _fSideGainBlue;

	root["MUXRate"] = _uMUXRate;
}

void CLightCalSettings::Load(const Json::Value &root)
{
	_fFrontGainRed = root.get("FrontGainRed", _fFrontGainRed).asFloat();
	_fFrontGainGreen = root.get("FrontGainGreen", _fFrontGainGreen).asFloat();
	_fFrontGainBlue = root.get("FrontGainBlue", _fFrontGainBlue).asFloat();
	
	_fSideGainRed = root.get("SideGainRed", _fSideGainRed).asFloat();
	_fSideGainGreen = root.get("SideGainGreen", _fSideGainGreen).asFloat();
	_fSideGainBlue = root.get("SideGainBlue", _fSideGainBlue).asFloat();
	
	_uMUXRate = root.get("MUXRate", _uMUXRate).asUInt();
}

