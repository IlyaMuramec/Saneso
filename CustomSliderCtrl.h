#pragma once

class CCustomSliderCtrl : public CSliderCtrl
{
public:
	CCustomSliderCtrl();
	
	void SetTopBottomValues(float top, float bottom);
	int GetValue();
	void SetValue(int value);
	float GetFloatValue();
	void SetFloatValue(float value);

private:
	float m_fTopValue;
	float m_fBottomValue;
};

