#pragma once

#include "json\json.h"

class CLightCalSettings
{
public:
	CLightCalSettings();
	~CLightCalSettings();

	void Save(Json::Value &root);
	void Load(const Json::Value &root);

	void GetFrontGain(float &red, float &green, float &blue)
	{
		red = _fFrontGainRed;
		green = _fFrontGainGreen;
		blue = _fFrontGainBlue;
	}

	void SetFrontGain(float red, float green, float blue)
	{
		_fFrontGainRed = red;
		_fFrontGainGreen = green;
		_fFrontGainBlue = blue;
	}

	void GetSideGain(float &red, float &green, float &blue)
	{
		red = _fSideGainRed;
		green = _fSideGainGreen;
		blue = _fSideGainBlue;
	}
	void SetSideGain(float red, float green, float blue)
	{
		_fSideGainRed = red;
		_fSideGainGreen = green;
		_fSideGainBlue = blue;
	}

	unsigned int GetMUXRate()			{ return _uMUXRate; }
	void SetMUXRate(unsigned int rate)	{ _uMUXRate = rate; }

private:
	float _fFrontGainRed;
	float _fFrontGainGreen;
	float _fFrontGainBlue;
	float _fSideGainRed;
	float _fSideGainGreen;
	float _fSideGainBlue;
	unsigned int _uMUXRate;
};

