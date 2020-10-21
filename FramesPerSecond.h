#pragma once

#include <queue>
#include "afxmt.h"

class CFramesPerSecond
{
public:
	CFramesPerSecond();
	~CFramesPerSecond();

	void NewFrame();
	double CalcFPS();

private:
	double m_dHistorySeconds;
	std::deque<double> m_vFrameTimes;
	LONGLONG m_llPerfFreq;
	CCriticalSection m_CriticalSection;
};

