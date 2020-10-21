#pragma once
#include <string>  // javqui
#include <map>

class ProcTimes
{
public:
	ProcTimes();
	~ProcTimes();

	void ClearTimes();
	void AddTime(std::string camera, std::string name, double time);
	void SaveTimes(std::string filename);

private:
	std::map<std::string, double> _times;
	std::map<std::string, int> _counts;
};

