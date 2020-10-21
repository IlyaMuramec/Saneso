#include "StdAfx.h"
#include "ProcTimes.h"
#include <fstream>
#include <sstream>

using namespace std;

ProcTimes::ProcTimes()
{
}


ProcTimes::~ProcTimes()
{
}

void ProcTimes::ClearTimes()
{
	// Reset the timers
	_times.clear();

}

void ProcTimes::AddTime(std::string camera, std::string name, double time)
{
	stringstream ss;
	ss << camera << "_" << name;
	if (_times.find(ss.str()) == _times.end())
	{
		_counts[ss.str()] = 1;
		_times[ss.str()] = time;
	}
	else
	{
		_counts[ss.str()]++;
		_times[ss.str()] += time;
	}
}

void ProcTimes::SaveTimes(string filename)
{
	// Dump the times
	ofstream file(filename.c_str());
	file << "Entry,Elapsed" << endl;

	std::map<std::string, double>::iterator it;
	for (it = _times.begin(); it != _times.end(); ++it)
	{
		float avg = it->second / _counts[it->first];
		file << it->first.c_str() << "," << avg << endl;
	}
	file.close();
	
	ClearTimes();
}

