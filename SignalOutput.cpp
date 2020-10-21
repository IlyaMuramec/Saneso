#include "stdafx.h"
#include "SignalOutput.h"
#include "SerialPort.h"
#include <iomanip>


log4cxx::LoggerPtr CSignalOutput::logger = log4cxx::Logger::getLogger("SignalOutput");



CSignalOutput::CSignalOutput()
{
	m_pSerialPort = new CSerialPort();
	_nConnectedComPort = -1;
	m_uTriggerDuration = 10;
}


CSignalOutput::~CSignalOutput()
{
	Disconnect();
}

int CSignalOutput::Connect(int comPort)
{
	if (_nConnectedComPort >= 0)
	{
		Disconnect();
	}

	// Attempt to conntect
	int retVal = TryComPort(comPort);

	return retVal;
}

int CSignalOutput::TryComPort(int comPort)
{
	LOG4CXX_INFO(logger, "Connecting to COM port " << comPort);

	int retVal = -1;

	m_CriticalSection.Lock();

	if (comPort > 0)
	{
		try
		{
			BOOL bOverlapped = FALSE;

			m_pSerialPort->Close();
			m_pSerialPort->Open(comPort, 9600, 
				CSerialPort::EvenParity, 8,
				CSerialPort::OneStopBit,
				CSerialPort::NoFlowControl,
				bOverlapped);

			if (m_pSerialPort->IsOpen())
			{
				SetDTR(false);

				LOG4CXX_INFO(logger, "COM port is open");
				_nConnectedComPort = comPort;
				retVal = 0;
			}
		}
		catch (...) {}
	}

	m_CriticalSection.Unlock();

	return retVal;
}


DWORD CSignalOutput::SendMesage(const void* lpSendBuf, DWORD dwSendCount, bool bLog = true)
{
	DWORD bytesSent = 0;

	try
	{
		if (_nConnectedComPort >= 0)
		{
			m_CriticalSection.Lock();

			// Add escape characters as needed
			std::vector<byte> buffer((const byte *)lpSendBuf, (const byte *)lpSendBuf + dwSendCount);
			if (bLog)
			{
				std::stringstream ss;
				ss << "Send [ ";
				for (int i = 0; i < buffer.size(); i++)
				{
					ss << std::showbase << std::setfill('0') << std::setw(2) << std::hex << (int)buffer[i] << " ";
				}
				ss << "]";
				LOG4CXX_DEBUG(logger, ss.str());
			}

			bytesSent = m_pSerialPort->Write(&buffer[0], buffer.size());

			m_CriticalSection.Unlock();
		}
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger, "Serial port send error");

		m_CriticalSection.Unlock();
		Disconnect();
	}

	return bytesSent;
}


void CSignalOutput::Disconnect()
{
	if (_nConnectedComPort < 0)
	{
		LOG4CXX_INFO(logger, "Already disconnected");
		return;
	}
	
	m_CriticalSection.Lock();

	LOG4CXX_INFO(logger, "Disconnecting");

	m_pSerialPort->Close();
	_nConnectedComPort = -1;

	LOG4CXX_INFO(logger, "Disconnected");

	m_CriticalSection.Unlock();
}

void CSignalOutput::SendTrigger()
{	
	SetDTR(true);

	Sleep(m_uTriggerDuration);

	SetDTR(false);
}

void CSignalOutput::SetBreak(bool val)
{
	if (_nConnectedComPort >= 0)
	{
		if (val)
		{
			m_pSerialPort->SetBreak();
		}
		else
		{
			m_pSerialPort->ClearBreak();
		}
	}
}

void CSignalOutput::SetDTR(bool val)
{
	if (_nConnectedComPort >= 0)
	{
		if (val)
		{
			m_pSerialPort->SetDTR();
		}
		else
		{
			m_pSerialPort->ClearDTR();
		}
	}
}

void CSignalOutput::SetRTS(bool val)
{
	if (_nConnectedComPort >= 0)
	{
		if (val)
		{
			m_pSerialPort->SetRTS();
		}
		else
		{
			m_pSerialPort->ClearRTS();
		}
	}
}