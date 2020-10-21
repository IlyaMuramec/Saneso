#pragma once

#include <windows.h>

class TimingData
{
public:
	TimingData();
	~TimingData();

	void Restart();
	double Elapsed_ms();

private:
	LONGLONG _freq;
	LONGLONG _start;
};

