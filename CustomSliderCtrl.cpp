#include "stdafx.h"
#include "CustomSliderCtrl.h"


CCustomSliderCtrl::CCustomSliderCtrl() : 
	m_fTopValue(100), 
	m_fBottomValue(0) 
{
}

void CCustomSliderCtrl::SetTopBottomValues(float top, float bottom)
{
	m_fTopValue = top;
	m_fBottomValue = bottom;
}

int CCustomSliderCtrl::GetValue()
{
	int pos = GetPos();
	float value = 0.01 * pos * (m_fBottomValue - m_fTopValue) + m_fTopValue;
	return round(value);
}

void CCustomSliderCtrl::SetValue(int value) 
{
	float pos = 100.0 * (value - m_fTopValue) / (m_fBottomValue - m_fTopValue);
	SetPos(round(pos));
}

float CCustomSliderCtrl::GetFloatValue()	
{
	int pos = GetPos();
	float value = 0.01 * pos * (m_fBottomValue - m_fTopValue) + m_fTopValue;
	return value;
}

void CCustomSliderCtrl::SetFloatValue(float value) 
{
	float pos = 100.0 * (value - m_fTopValue) / (m_fBottomValue - m_fTopValue);
	SetPos(round(pos));
}