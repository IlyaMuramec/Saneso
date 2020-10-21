#pragma once

#include "afxmt.h"
#include <log4cxx\logger.h>

class CSerialPort;



class CSignalOutput
{
public:
	CSignalOutput();
	~CSignalOutput();

	int Connect(int comPort);
	int GetConnectedComPort() { return _nConnectedComPort; }

	UINT GetTriggerDuration()				{ return m_uTriggerDuration; }
	void SetTriggerDuration(UINT value)		{ m_uTriggerDuration = value; }
	
	void SendTrigger();
	void SetBreak(bool val);
	void SetDTR(bool val);
	void SetRTS(bool val);

private:
	static log4cxx::LoggerPtr logger;

	int TryComPort(int comPort);

	DWORD SendMesage(const void* lpSendBuf, DWORD dwSendCount, bool bLog);
	void Disconnect();

	CSerialPort *m_pSerialPort;
	CCriticalSection m_CriticalSection;

	int _nConnectedComPort;
	UINT m_uTriggerDuration;
};

