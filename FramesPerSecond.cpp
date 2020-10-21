#include "stdafx.h"
#include "FramesPerSecond.h"

#define MAX_FRAME_COUNT 1000

CFramesPerSecond::CFramesPerSecond()
{
	m_dHistorySeconds = 5.0;

	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	m_llPerfFreq = li.QuadPart;
}


CFramesPerSecond::~CFramesPerSecond()
{
}

void CFramesPerSecond::NewFrame()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	double frameTime = (double) li.QuadPart / m_llPerfFreq;

	m_CriticalSection.Lock();

	if (m_vFrameTimes.size() > MAX_FRAME_COUNT)
	{
		m_vFrameTimes.pop_back();
	}
	m_vFrameTimes.push_front(frameTime);

	m_CriticalSection.Unlock();
}

double CFramesPerSecond::CalcFPS()
{
	double dFPS = 0;

	if (!m_vFrameTimes.empty())
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		double dCurrentTime = (double) li.QuadPart / m_llPerfFreq;

		m_CriticalSection.Lock();

		double dAvgSum = 0.0;
		int nAvgCount = 0;
		double  dLastTime = m_vFrameTimes[0];
		double  dThisTime = 0;
		for (int i = 0; i < m_vFrameTimes.size(); i++)
		{
			dThisTime = m_vFrameTimes[i];
			if ((dCurrentTime - dThisTime) < m_dHistorySeconds)
			{
				nAvgCount++;
			}
			else
			{
				break;
			}
		}

		m_CriticalSection.Unlock();

		dFPS = (double)nAvgCount / m_dHistorySeconds;
	}
	
	return dFPS;
}
