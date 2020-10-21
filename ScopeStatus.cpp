#include "stdafx.h"
#include "ScopeStatus.h"

log4cxx::LoggerPtr  CScopeStatus::logger(log4cxx::Logger::getLogger("ScopeStatus"));

// From Redmine #19201
//Bit 0 = scope is present
//Bit 1 = scope current test ok
//Bit 2 = scope EEPROM communication is ok
//Bit 3 = scope LED driver communication is ok
//Bit 4 = scope MIPI repeater I2C communication is ok
//Bit 5 = scope CAM5 I2C communication is ok
//Bit 6 = microcontroller is recovering i2c communications

#define SCOPE_PRESENT	0
#define CURRENT_OK		1
#define EEPROM_OK		2
#define LED_OK			3
#define MIPI_I2C_OK			4
#define CAM5_I2C_OK			5
#define SCOPE_RECOVERING	6

using namespace std;

CScopeStatus::CScopeStatus()
{
	m_LastStatus.set(SCOPE_PRESENT);
}


CScopeStatus::~CScopeStatus()
{
}

void CScopeStatus::UpdateStatus(bitset<NUM_SCOPE_STATUS_BITS> status)
{
	status = status.set(CURRENT_OK, false);

	if (status != m_LastStatus)
	{
		LogStatus(status);

		m_LastStatus = status;
	}
}

bool CScopeStatus::IsScopeConnected()
{
	return m_LastStatus.test(SCOPE_PRESENT);
}

bool CScopeStatus::IsScopeCommOK()
{
	return (m_LastStatus.test(MIPI_I2C_OK) && m_LastStatus.test(CAM5_I2C_OK));
}

bool CScopeStatus::IsScopeRecovering()
{
	return m_LastStatus.test(SCOPE_RECOVERING);
}

void CScopeStatus::LogStatus(bitset<NUM_SCOPE_STATUS_BITS> status)
{
	LOG4CXX_INFO(logger, "ScopePresent[" << (status.test(SCOPE_PRESENT) ? "Y" : "N") << "] "
						//<< "CurrentTestOK[" << (status.test(CURRENT_OK) ? "Y" : "N") << "] "
						<< "EEPROMCommOK[" << (status.test(EEPROM_OK) ? "Y" : "N") << "] "
						<< "LEDCommOK[" << (status.test(LED_OK) ? "Y" : "N") << "] "
						<< "MIPICommOK[" << (status.test(MIPI_I2C_OK) ? "Y" : "N") << "] "
						<< "CAM5CommOK[" << (status.test(CAM5_I2C_OK) ? "Y" : "N") << "] "
						<< "Recovering[" << (status.test(SCOPE_RECOVERING) ? "Y" : "N") << "]");
}