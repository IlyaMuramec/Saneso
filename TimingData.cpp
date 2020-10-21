#include "StdAfx.h"
#include "TimingData.h"


TimingData::TimingData()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	_freq = li.QuadPart;

	Restart();
}


TimingData::~TimingData()
{
}

void TimingData::Restart()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	_start = li.QuadPart;
}

double TimingData::Elapsed_ms()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - _start) / _freq;
}
