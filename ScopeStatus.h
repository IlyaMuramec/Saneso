#pragma once

#include <log4cxx\logger.h>
#include <bitset>

#define NUM_SCOPE_STATUS_BITS 7

class CScopeStatus
{
public:
	CScopeStatus();
	~CScopeStatus();


	void UpdateStatus(std::bitset<NUM_SCOPE_STATUS_BITS> status);
	bool IsScopeConnected();
	bool IsScopeCommOK();
	bool IsScopeRecovering();

private:
	void LogStatus(std::bitset<NUM_SCOPE_STATUS_BITS> status);

	static log4cxx::LoggerPtr logger;
	std::bitset<NUM_SCOPE_STATUS_BITS> m_LastStatus;
};

