#include "stdafx.h"
#include "MicroController.h"
#include "NGEuserApp.h"
#include "ScopeStatus.h"
#include "SerialPort.h"
#include <iomanip>

#include <Windows.h>
#include <setupapi.h>
#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID

log4cxx::LoggerPtr CMicroController::logger = log4cxx::Logger::getLogger("MicroController");

#pragma comment (lib, "setupapi.lib")

// include DEVPKEY_Device_BusReportedDeviceDesc from WinDDK\7600.16385.1\inc\api\devpropdef.h
#ifdef DEFINE_DEVPROPKEY
#undef DEFINE_DEVPROPKEY
#endif

#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY DECLSPEC_SELECTANY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }

// include DEVPKEY_Device_BusReportedDeviceDesc from WinDDK\7600.16385.1\inc\api\devpkey.h
DEFINE_DEVPROPKEY(DEVPKEY_Device_Manufacturer, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 13);    // DEVPROP_TYPE_STRING

typedef BOOL(WINAPI *FN_SetupDiGetDevicePropertyW)(
	__in       HDEVINFO DeviceInfoSet,
	__in       PSP_DEVINFO_DATA DeviceInfoData,
	__in       const DEVPROPKEY *PropertyKey,
	__out      DEVPROPTYPE *PropertyType,
	__out_opt  PBYTE PropertyBuffer,
	__in       DWORD PropertyBufferSize,
	__out_opt  PDWORD RequiredSize,
	__in       DWORD Flags
	);

const int NUM_BOARDS = 5;

// Host to MC messages
#define RESET_MSG               0x00
#define GET_VERSION             0x01
#define CAMERA_MSG              0x02 
#define SET_LED_LEVEL           0x03
#define SET_SHADOWCAST          0x04
#define READ_EEPROM             0x05
#define WRITE_EEPROM            0x06
#define TOGGLE_LIGHTS           0x07
#define GET_STATUS              0x08
#define CAMERA_RESET_OUTPUT     0x09
#define SET_LED_LEVELS			0x0A
#define TEST_HW_CMD				0x0F

//eeprom commands to r/w serial number (backwards compatible)
#define EEP_RW_BOX_SN   0x00
#define EEP_RW_SCOPE_SN 0x01
//eeprom commands to r/w any addr ( new ))
#define EEP_RW_BOX      0x10
#define EEP_RW_SCOPE    0x11
//Camera Reset msg values
#define CAM_HW_RESET_CLEAR		0x00
#define CAM_HW_RESET_ACTIVE		0x01

// MC to Host responses
#define GET_VERSION_RESP            0x11
#define CAMERA_MSG_RESP             0x12              
#define SET_LED_LEVEL_RESP          0x13
#define SET_SHADOWCAST_RESP         0x14
#define READ_EEPROM_RESP            0x15
#define WRITE_EEPROM_RESP           0x16
#define TOGGLE_LIGHTS_RESP          0x17
#define GET_STATUS_RESP             0x18
#define CAMERA_RESET_OUTPUT_RESP    0x19
#define SET_LED_LEVELS_RESP			0x1A
#define TEST_HW_CMD_RESP			0x1F


#define START_END_CHAR				0x7E

#define SPI_INIT_CAMERA_CMD			0x4A
#define SPI_WRITE_CMD				0x4E
#define SPI_WRITE_CMD_ONE_TIME		0x4F
#define SPI_READ_CMD				0x57

#define PCOM_BUF_SIZE				64
#define PCOM_PAYLOAD_SIZE			(PCOM_BUF_SIZE - 2)
#define PCOM_EEPROM_RW_LIMIT		32 // Don't go over 0xFF boundary

#define EEP_MAX_ADDR				0x0003ffff 
#define EEP_DATA_LENGTH_LOC			0x00000100 // Leave the first bytes for the SN 
#define EEP_DATA_START_LOC			0x00000200 // Leave plenty of bytes to store the data length

#define USB_DESC			TEXT("Serial")
#define USB_MANUFACTURER	TEXT("Microchip")


enum ReceiveState
{
	eStartChar,
	eType,
	eSize,
	eData,
	eEnd
};

CMicroController::CMicroController(CScopeStatus *pScopeStatus)
{
	m_pScopeStatus = pScopeStatus;
	m_pSerialPort = new CSerialPort();
	_bExiting = false;
	_bConnected = false;
	_nConnectedComPort = -1;
	_hReceiveThread = NULL;
	_hSimulateThread = NULL;
	_hMonitorThread = NULL;
	m_vShutterTimes = std::vector<int>(NUM_BOARDS + 1);
	m_vGains = std::vector<int>(NUM_BOARDS + 1);
	m_sVersion = "Not Connected";
	_nShutterMonitorBoard = 0;
}


CMicroController::~CMicroController()
{
	EndThreads();
	Disconnect();
}

int CMicroController::Connect()
{
	if (_bConnected)
		return 0;

	int retryCount = 5;
	for (int i = 0; i < retryCount; i++)
	{
		CUIntArray comPortNumbers;
		CSerialPort::EnumerateSerialPorts(comPortNumbers);

		_nConnectedComPort = -1;
		for (int i = 0; i < comPortNumbers.GetSize(); i++)
		{
			int comPort = (int)comPortNumbers.ElementAt(i);

			// Attempt to conntect
			if (TryComPort(comPort) == 0)
			{
				_nConnectedComPort = comPort;
				break;
			}
		}

		if (_nConnectedComPort > -1)
		{
			break;
		}
		else if (i == 0)
		{
			ToggleUSBDeviceState();
		}
		else
		{
			Sleep(1000);
		}
	}
}


int CMicroController::TryComPort(int comPort)
{
	LOG4CXX_INFO(logger, "Connecting to COM port " << comPort);
	//LOG4CXX_INFO(logger, printf("Connecting to COM port %d",comPort)); // javqui

	int retVal = -1;

	m_CriticalSection.Lock();

	if (comPort > 0)
	{
		try
		{
			BOOL bOverlapped = FALSE;

			m_pSerialPort->Close();
			m_pSerialPort->Open(comPort, 115200, 
				CSerialPort::NoParity, 8,
				CSerialPort::OneStopBit,
				CSerialPort::XonXoffFlowControl,
				bOverlapped);

			LOG4CXX_INFO(logger, "COM port is open");

			if (m_pSerialPort->IsOpen())
			{
				COMMTIMEOUTS timeouts;
				m_pSerialPort->GetTimeouts(timeouts);

				timeouts.ReadIntervalTimeout = 0;
				timeouts.ReadTotalTimeoutConstant = 1000;


/* multi tests javqui to fix the log4cxx issues
				if (logger->isInfoEnabled()) {
					std::ostringstream ossj;
					ossj << "SetTimeouts:" 
						<< " ReadIntervalTimeout = " << timeouts.ReadIntervalTimeout
						<< " ReadTotalTimeoutConstant = " << timeouts.ReadTotalTimeoutConstant
						<< " ReadTotalTimeoutMultiplier = " << timeouts.ReadTotalTimeoutMultiplier
						<< " WriteTotalTimeoutConstant = " << timeouts.WriteTotalTimeoutConstant
						<< " WriteTotalTimeoutMultiplier = " << timeouts.WriteTotalTimeoutMultiplier;
					logger->forcedLog(::log4cxx::Level::getInfo(), ossj.str(), LOG4CXX_LOCATION);
				}

				LOG4CXX_INFO_JAVQUI(logger, "SetTimeouts:"
					<< " ReadIntervalTimeout = " << timeouts.ReadIntervalTimeout
					<< " ReadTotalTimeoutConstant = " << timeouts.ReadTotalTimeoutConstant
					<< " ReadTotalTimeoutMultiplier = " << timeouts.ReadTotalTimeoutMultiplier
					<< " WriteTotalTimeoutConstant = " << timeouts.WriteTotalTimeoutConstant
					<< " WriteTotalTimeoutMultiplier = " << timeouts.WriteTotalTimeoutMultiplier);

	*/			
				LOG4CXX_INFO(logger, "SetTimeouts:" 
					<< " ReadIntervalTimeout = " << timeouts.ReadIntervalTimeout
					<< " ReadTotalTimeoutConstant = " << timeouts.ReadTotalTimeoutConstant
					<< " ReadTotalTimeoutMultiplier = " << timeouts.ReadTotalTimeoutMultiplier
					<< " WriteTotalTimeoutConstant = " << timeouts.WriteTotalTimeoutConstant
					<< " WriteTotalTimeoutMultiplier = " << timeouts.WriteTotalTimeoutMultiplier);
					
				
				//LOG4CXX_INFO(logger, "SetTimeouts:" << " ReadIntervalTimeout = " << timeouts.ReadIntervalTimeout );  // testing
				//LOG4CXX_INFO(logger, "SetTimeouts:" << " ReadTotalTimeoutConstant = " << timeouts.ReadTotalTimeoutConstant);  // testing


				//LOG4CXX_INFO(logger, printf("SetTimeouts: ReadIntervalTimeout=%d ReadTotalTimeoutConstant=%d ReadTotalTimeoutMultiplier=%d WriteTotalTimeoutConstant=%d WriteTotalTimeoutMultiplier=%d",
				//	timeouts.ReadIntervalTimeout,timeouts.ReadTotalTimeoutConstant,timeouts.ReadTotalTimeoutMultiplier,timeouts.WriteTotalTimeoutConstant,timeouts.WriteTotalTimeoutMultiplier));

				m_pSerialPort->SetTimeouts(timeouts);

				retVal = ReadVersion();

				LOG4CXX_INFO(logger, "ReadVersion returned "<< retVal);
				//LOG4CXX_INFO(logger, printf("ReadVersion returned %d",retVal));

				if (retVal != 0)
				{
					m_pSerialPort->Close();
				}
				else
				{
					m_pSerialPort->Set0WriteTimeout();

					// Update the status so that scope connection status is up to date
					ReadStatus();
				}
			}
		}
		catch (...) {}


	}

	m_CriticalSection.Unlock();

	if (retVal == 0)
	{
		OnConnected();
	}

	return retVal;
}

int CMicroController::GetShutterValue(int board)
{
	int value = -1;
	if ((board >= 0) && (board <= NUM_BOARDS))
	{
		value = m_vShutterTimes[board];
	}
	return value;
}

int CMicroController::GetGainValue(int board)
{
	int value = -1;
	if ((board >= 0) && (board <= NUM_BOARDS))
	{
		value = m_vGains[board];
	}
	return value;
}

DWORD CMicroController::SendMesage(const void* lpSendBuf, DWORD dwSendCount, void* lpRecvBuf, DWORD dwRecvCount, bool bLog = true)
{
	DWORD bytesRead = 0;

	try
	{
		if (_bConnected)
		{
			m_CriticalSection.Lock();

			// Add escape characters as needed
			std::vector<byte> buffer = PreprocessForSend(lpSendBuf, dwSendCount);
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

			m_pSerialPort->Write(&buffer[0], buffer.size());

			// Need to give up thread of exectution for rapid fire messages
			Sleep(0);

			if (dwRecvCount > 0)
			{
				CMessage *pMsg = m_ReceiveQueue.Pop();
				if (pMsg != NULL)
				{
					// Remove escape characters as needed
					std::vector<byte> buffer = PreprocessFromReceive(&pMsg->rawData[0], pMsg->rawData.size());
					
					bytesRead = buffer.size();
					memcpy(lpRecvBuf, &buffer[0], bytesRead);
					delete pMsg;
				}

				if (bLog)
				{
					std::stringstream ssRecv;
					ssRecv << "Response [ ";
					for (int i = 0; i < bytesRead; i++)
					{
						ssRecv << std::showbase << std::setfill('0') << std::setw(2) << std::hex << (int)((const BYTE*)lpRecvBuf)[i] << " ";
					}
					ssRecv << "]";
					LOG4CXX_DEBUG(logger, ssRecv.str());
				}
			}

			m_CriticalSection.Unlock();
		}
	}
	catch (...)
	{
		LOG4CXX_ERROR(logger, "Serial port send error");

		m_CriticalSection.Unlock();
		Disconnect();
	}

	return bytesRead;
}

void CMicroController::EndThreads()
{
	_bExiting = true;

	if (_hReceiveThread != NULL)
	{
		WaitForSingleObject(_hReceiveThread, INFINITE);
		_hReceiveThread = NULL;
	}
	LOG4CXX_INFO(logger, "Receive thread complete");

	if (_hMonitorThread != NULL)
	{
		WaitForSingleObject(_hMonitorThread, INFINITE);
		_hMonitorThread = NULL;
	}
	LOG4CXX_INFO(logger, "Monitor thread complete");

	if (_hSimulateThread != NULL)
	{
		WaitForSingleObject(_hSimulateThread, INFINITE);
		_hSimulateThread = NULL;
	}
	LOG4CXX_INFO(logger, "Simulate thread complete");

	_bExiting = false;

}

void CMicroController::OnConnected()
{
	// Restart the receive threads
	EndThreads();

	_bConnected = true;
	
	// Create the receive thread
	DWORD dwThreadID;
	_hReceiveThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CMicroController::ReceiveThreadProc,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier

	_hMonitorThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CMicroController::MonitorThreadProc,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier

	// Call the callbacks
	std::map<ConnectCallbackType, void*>::iterator it;
	for (it = m_ConnectCallbacks.begin(); it != m_ConnectCallbacks.end(); ++it)
	{
		it->first(it->second);
	}
}

void CMicroController::Disconnect()
{
	if (!_bConnected)
	{
		LOG4CXX_INFO(logger, "Already disconnected");
		return;
	}
	
	m_CriticalSection.Lock();

	LOG4CXX_INFO(logger, "Disconnecting");

	m_pSerialPort->Close();
	_bConnected = false;
	_nConnectedComPort = -1;
	m_sVersion = "Not Connected";

	ToggleUSBDeviceState();

	LOG4CXX_INFO(logger, "Disconnected");

	m_CriticalSection.Unlock();
}


UINT CMicroController::ReceiveLoop()
{
	// Update the timeouts
	m_pSerialPort->Set0ReadTimeout();
	
	m_ReceiveQueue.Empty();
	
	while (!_bExiting)
	{
		char lpRecvBuf;
		int bytesRead;
		ReceiveState state = eStartChar;
		CMessage *pMsg = NULL;
		while (_bConnected)
		{
			bytesRead = 0;
			
			try
			{
				bytesRead = m_pSerialPort->Read(&lpRecvBuf, 1);
			}
			catch (...)
			{
				LOG4CXX_ERROR(logger, "Serial port read error");
				Disconnect();
			}

			if (bytesRead == 0)
			{
				break;
			}

			// Push raw data for verification
			if (pMsg)
			{
				pMsg->rawData.push_back(lpRecvBuf);
			}

			switch (state)
			{
			case eStartChar:
				if (lpRecvBuf == START_END_CHAR)
				{
					pMsg = new CMessage();
					pMsg->rawData.push_back(lpRecvBuf); // Push the first char
					state = eType;
				}
				break;
			case eType:
				pMsg->type = lpRecvBuf;
				state = eSize;
				break;
			case eSize:
				pMsg->size = lpRecvBuf;
				pMsg->msgData.reserve(lpRecvBuf);
				state = eData;
				break;
			case eData:
				pMsg->msgData.push_back(lpRecvBuf);
				if (pMsg->msgData.size() == pMsg->size)
				{
					state = eEnd;
				}
				break;
			case eEnd:
				if (lpRecvBuf == START_END_CHAR)
				{
					std::stringstream ssRecv;
					ssRecv << "Recv [ ";
					for (int i = 0; i < pMsg->rawData.size(); i++)
					{
						ssRecv << std::showbase << std::setfill('0') << std::setw(2) << std::hex << (int)pMsg->rawData[i] << " ";
					}
					ssRecv << "]";
					LOG4CXX_DEBUG(logger, ssRecv.str());

					bool bResponse = ((pMsg->type >= 0x11) && (pMsg->type < 0x21));
					bool bAync = ((pMsg->type >= 0x21) && (pMsg->type != MC_ERROR_MESSAGE));

					if (pMsg->type == MC_ERROR_MESSAGE)
					{
						// 1 byte error message
						LOG4CXX_ERROR(logger, "Error = " << std::hex << (int)pMsg->msgData[0]);
						//LOG4CXX_ERROR(logger, printf("Error =0x%x",(int)pMsg->msgData[0]));

						// Error was in reponse to a message so send reponse
						bResponse = true;
					}

					if (bAync || (pMsg->type == GET_STATUS_RESPONSE))
					{
						if (pMsg->type == MC_CONTROL_CHANGED)
						{
							// Handset or from panel control change
							int control = pMsg->msgData[0];
							int value = pMsg->msgData[1];
							LOG4CXX_DEBUG(logger, "Handset message received Control = " << std::hex << control << " Value = " << std::hex << value);
							//LOG4CXX_DEBUG(logger, printf("Handset message received Control= 0x%x Value=0x%x",control,value));
						}
						else if ((pMsg->type == MC_STATUS_MESSAGE)
							|| (pMsg->type == GET_STATUS_RESPONSE))
						{
							std::stringstream ss;
							ss << std::hex << (int)pMsg->msgData[0] << " "
								<< std::hex << (int)pMsg->msgData[1] << " "
								<< std::hex << (int)pMsg->msgData[2] << " "
								<< std::hex << (int)pMsg->msgData[3];

							m_sStatus = ss.str();

							// 4 byte status message
							/*LOG4CXX_DEBUG(logger, "Status = "
								<< std::hex << (int)pMsg->msgData[0] << " "
								<< std::hex << (int)pMsg->msgData[1] << " "
								<< std::hex << (int)pMsg->msgData[2] << " "
								<< std::hex << (int)pMsg->msgData[3]);*/
						}
						
						// Call all the listeners
						NotifyMessageListeners(pMsg);
					}

					if (bResponse)
					{
						// Make a copy of the message for queue
						CMessage *pMsgCopy = new CMessage(*pMsg);

						// Push it onto the queue for anyone waiting for a response
						m_ReceiveQueue.Push(pMsgCopy);
					}

					delete pMsg;
					pMsg = NULL;
					state = eStartChar;
				}
				break;
			}
		}
	}

	return 0;
}

void CMicroController::SimulateControlMessages()
{
	// Create the simulate thread
	DWORD dwThreadID;
	_hSimulateThread = CreateThread(
		NULL,         // default security attributes
		0,            // default stack size
		(LPTHREAD_START_ROUTINE)&CMicroController::SimulateThreadProc,
		(LPVOID)this,         // no thread function arguments
		0,            // default creation flags
		&dwThreadID); // receive thread identifier
}

UINT CMicroController::SimulateLoop()
{
	bool bOn = 0;
	int nVal = 0;

	CMessage *pMsg;

	while (!_bExiting)
	{
#if 1		
		bOn = !bOn;
		nVal++;
		if (nVal > 10)
			nVal = 0;
		if (nVal < 0)
			nVal = 10;

		pMsg = new CMessage();
		pMsg->type = MC_CONTROL_CHANGED;
		pMsg->size = 2;
		pMsg->msgData.push_back(CTRL_REMOTE_2);
		pMsg->msgData.push_back(bOn);

		// Handset or from panel control change
		int control = pMsg->msgData[0];
		int value = pMsg->msgData[1];
		LOG4CXX_DEBUG(logger, "Handset simulation message received Control = " << std::hex << control << " Value = " << std::hex << value);
		//LOG4CXX_DEBUG(logger, printf("Handset simulation message received Control = 0x%x Value= 0x%x",control,value)); // javqui
		
#else
		bOn = !bOn;

		pMsg = new CMessage();
		pMsg->type = MC_STATUS_MESSAGE;
		pMsg->size = 4;
		pMsg->msgData.resize(4);
		pMsg->msgData[0] = bOn;
#endif
		// Call all the listeners
		NotifyMessageListeners(pMsg);
		delete pMsg;

		Sleep(1000);
	}

	return 0;
}

UINT CMicroController::MonitorLoop()
{
	while (!_bExiting)
	{
		if (!_bConnected)
		{
			continue;
		}

		if (ReadStatus() == 0)
		{
			_heartbeatsPerSecond.NewFrame();
		}

		if ((_nShutterMonitorBoard > 0) 
			&& (_nShutterMonitorBoard <= 5))  //TODO Make sure the camera is connected
		{
			ReadShutterValue(_nShutterMonitorBoard);
		}

		if ((_nGainMonitorBoard > 0)
			&& (_nGainMonitorBoard <= 5))  //TODO Make sure the camera is connected
		{
			//ReadGainValue(_nGainMonitorBoard);  AEG doing AGC in app
		}

		Sleep(1000);
	}

	return 0;
}

void CMicroController::NotifyMessageListeners(CMessage *pMsg)
{
	if (pMsg->type == MC_CONTROL_CHANGED)
	{
		std::stringstream ss;
		for (int i = 0; i < pMsg->msgData.size(); i++)
		{
			ss << std::showbase << std::setfill('0') << std::setw(2) << std::hex << (int)pMsg->msgData[i] << " ";
		}
		LOG4CXX_INFO(logger, "Forwarding Control Message: " << ss.str());
	}
	
	std::map<MessageCallbackType, void*>::iterator it;
	for (it = m_MessageCallbacks.begin(); it != m_MessageCallbacks.end(); ++it)
	{
		it->first(it->second, pMsg);
	}
}


void CMicroController::ReadAllShutterTimes()
{
	for (int i = 1; i < NUM_BOARDS + 1; i++)
	{
		ReadShutterValue(i);
	}
}

void CMicroController::SendReset()
{
	byte msgSend[4];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = RESET_MSG;
	msgSend[2] = 0x00;
	msgSend[3] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 4, &msgRecv[0], 0);
}


//Send(Reset and Reflash cmd) :
//0x7e 0x00 0x03 0xa5 0xhb 0xlb 0x7e
//Where :
//	  0x00 = reset command
//	  0x03 = message length
//	  0xa5 = reflash command
//	  0xhb  0xlb = [0xhblb] timeout to start reflash process in 1ms ticks.i.e 0x75 0x30 = 30000 = 30secs.
//	  If reflash communication from HIDBootloader.exe SW doesn’t start before timeout then bootloader exits to main app if present.
//
//There are two special values :
//0x00 0x00 = Default to hardcoded bootloader timeout(currently 15 secs.)
//0xff 0xff = No timeout, stay in bootloader mode until reset command from HIDBootloader.exe SW.
void CMicroController::SendReflash()
{
	byte msgSend[7];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = RESET_MSG;
	msgSend[2] = 0x03;
	msgSend[3] = 0xA5;
	msgSend[4] = 0x75;
	msgSend[5] = 0x30;
	msgSend[6] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 7, &msgRecv[0], 0);
}

int CMicroController::ReadVersion()
{
	int retVal = -1;

	LOG4CXX_INFO(logger, "Getting version");

	byte msgSend[4];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = GET_VERSION;
	msgSend[2] = 0x00;
	msgSend[3] = START_END_CHAR;

	// Add escape characters as needed
	std::vector<byte> buffer = PreprocessForSend(msgSend, 4);
	m_pSerialPort->Write(&buffer[0], buffer.size());

	byte msgRecv[128];
	DWORD bytesRead = m_pSerialPort->Read(&msgRecv, 128);
	if ((bytesRead > 2) && (msgRecv[0] == START_END_CHAR) && (msgRecv[1] == GET_VERSION_RESP))
	{
		int length = msgRecv[2];
		m_sVersion = "";
		for (int i = 0; i < length; i++)
		{
			m_sVersion += (char)msgRecv[3 + i];
		}
		LOG4CXX_INFO(logger, "Version is " << m_sVersion);

		retVal = 0;
	}

	return retVal;
}

int CMicroController::ReadStatus()
{
	int retVal = -1;

	//LOG4CXX_INFO(logger, "Getting status");

	byte msgSend[4];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = GET_STATUS;
	msgSend[2] = 0x00;
	msgSend[3] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 4, &msgRecv[0], 8, true);
	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == GET_STATUS_RESP))
	{
		//LOG4CXX_INFO(logger, "Status was returned");
		int pos = m_sVersion.find_last_of('.');
		if (pos > 0)
		{
			string numString = m_sVersion.substr(pos + 1);
			int numValue = atoi(numString.c_str());
			if (numValue >= 65180285)
			{
				m_pScopeStatus->UpdateStatus(msgRecv[4]);
			}
		}
		retVal = 0;
	}

	return retVal;
}



int CMicroController::ReadShutterValue(int board)
{
	int retVal = -1;

	LOG4CXX_INFO(logger, "Reading Shutter value on board " << board);
	//LOG4CXX_INFO(logger, printf("Reading Shutter value on board %d",board)); // javqui

	unsigned short shutter;
	retVal = ReadCameraRegister(board, 0x3501, shutter);
	if (retVal == 0)
	{
		m_vShutterTimes[board] = shutter;

		LOG4CXX_INFO(logger, "Shutter value is " << shutter << " on board " << board);
		//LOG4CXX_INFO(logger, printf("Shutter value is %d on board %d",shutter,board));
	}

	return retVal;
}

int CMicroController::ReadGainValue(int board)
{
	int retVal = -1;

	unsigned short gainLow = -1;
	unsigned short gainHigh = -1;

	LOG4CXX_INFO(logger, "Reading Gain value on board " << board);
	//LOG4CXX_INFO(logger, printf("Reading Gain value on board %d", board)); // javqui

	unsigned short shutter;
	retVal = ReadCameraRegister(board, 0x350B, gainLow);
	retVal |= ReadCameraRegister(board, 0x350A, gainHigh);
	
	if (retVal == 0)
	{
		int gain = ((gainHigh << 8) + gainLow);
		m_vGains[board] = gain;

		LOG4CXX_INFO(logger, "Gain value is " << gain << " on board " << board);
		//LOG4CXX_INFO(logger, printf("Gain value is %d on board %d",gain,board)); // javqui
	}

	return retVal;
}

int CMicroController::SetTestPattern(int board, bool enable)
{
	int retVal = -1;

	LOG4CXX_INFO(logger, "Setting test pattern to " << (enable ? "On" : "Off")
		<< " on board " << board);

	unsigned int reg;
	unsigned short val;
	if (m_CameraChipMap[board] == OV426)
	{
		reg = 0x5001;
		val = enable ? 0x82 : 0x0;
	}
	else
	{
		reg = 0x5080;
		val = enable ? 0x80 : 0x0;
	}

	retVal = WriteCameraRegister(board, reg, val);

	if (retVal == 0)
	{
		LOG4CXX_INFO(logger, "Test pattern was set");
		retVal = 0;
	}

	return retVal;
}

int CMicroController::SetAutoShutterGain(int board, bool autoShutter, bool autoGain)
{
	int retVal = -1;

	LOG4CXX_INFO(logger, "Setting auto shutter(" << (autoShutter ? "On" : "Off") 
		<< ") and gain(" << (autoGain ? "On" : "Off")
		<< ") on board " << board);

	byte val = ((autoShutter ? 0x0 : 0x1) | (autoGain ? 0x0 : 0x2));
	retVal = WriteCameraRegister(board, 0x3503, val);

	if (retVal == 0)
	{
		LOG4CXX_INFO(logger, "Auto shutter and gain were set");
		retVal = 0;
	}

	return retVal;
}

int CMicroController::SetShutterValue(int board, int value)
{
	int retVal = -1;

	LOG4CXX_INFO(logger, "Setting shutter value to " << value
		<< " on board " << board);

	retVal = WriteCameraRegister(board, 0x3501, (value & 0xFF));

	if (retVal == 0)
	{
		LOG4CXX_INFO(logger, "Shutter value was set");
		retVal = 0;
	}

	return retVal;
}

int CMicroController::SetGainValue(int board, int value)
{
	int retVal = -1;

	m_vGains[board] = value; //  Storing last gain for AGC

	LOG4CXX_DEBUG(logger, "Setting gain value to " << value
		<< " on board " << board);

	retVal = WriteCameraHighLowRegister(board, 0x350A, 0x350B, value);

	if (retVal == 0)
	{
		LOG4CXX_DEBUG(logger, "Gain value was set");
	}

	return retVal;
}

int CMicroController::SetWhiteBalanceEnable(int board, bool enable)
{
	int retVal = -1;

	LOG4CXX_INFO(logger, "Setting white balance enable to " << (enable?"1":"0") 
		<< " on board " << board);

	
	if (m_CameraChipMap[board] == OV426)
	{
		int value = 0x1D; //default for reg 0x5000
		if (enable)
			value |= (1 << 0);
		else
			value &= ~(1 << 0);

		retVal = WriteCameraRegister(board, 0x5000, value);
	}
	else
	{
		int value = 0x2F; //default for reg 0x5000
		if (enable)
			value |= (1 << 4);
		else
			value &= ~(1 << 4);

		retVal =  WriteCameraRegister(board, 0x5000, value);
	}

	if (retVal == 0)
	{
		LOG4CXX_INFO(logger, "White balance enable was set");
	}

	return retVal;
}

int CMicroController::SetWhiteBalanceGains(int board, float red, float green, float blue)
{
	int retVal = -1;

	LOG4CXX_INFO(logger, "Setting RGB gains to " << red << " " << green << " " << blue << " on board " << board);
	//LOG4CXX_INFO(logger, printf("Setting RGB gains to R:%.6f G:%.6f B:%.6f on board %d",red,green,blue,board));

	if (m_CameraChipMap[board] == OV426)	
	{
		//Red 0x5186 (H) + 0x5187 (L)
		//Green 0x5188 (H) + 0x5189 (L)
		//Blue 0x518A (H) + 0x518B (L)
		retVal = WriteCameraHighLowRegister(board, 0x5186, 0x5187, (int)round(red * 1024));
		retVal |= WriteCameraHighLowRegister(board, 0x5188, 0x5189, (int)round(green * 1024));
		retVal |= WriteCameraHighLowRegister(board, 0x518A, 0x518B, (int)round(blue * 1024));
	}
	else
	{
		retVal = WriteCameraHighLowRegister(board, 0x5180, 0x5181, std::min(0x3FF, (int)round(red * 256)));
		retVal |= WriteCameraHighLowRegister(board, 0x5182, 0x5183, std::min(0x3FF, (int)round(green * 256)));
		retVal |= WriteCameraHighLowRegister(board, 0x5184, 0x5185, std::min(0x3FF, (int)round(blue * 256)));
	}

	return retVal;
}

int CMicroController::ReadCameraRegister(int board, unsigned int reg, unsigned short &value)
{
	int retVal = -1;

	if (m_CameraChipMap[board] == SIMULATED)
	{
		LOG4CXX_INFO(logger, "Ignoring read register on simulated camera board " << board);

		return retVal;
	}

	LOG4CXX_INFO(logger, "Reading register " << std::hex << std::showbase << reg
		<< " on board " << std::dec << board);

	// 0x7e 0x02 0x06 0x4E 0x01 0x01 0xHH 0xLL 0xVV 0x7e
	byte msgSend[9];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = CAMERA_MSG;
	msgSend[2] = 0x05;
	msgSend[3] = SPI_READ_CMD;
	msgSend[4] = 0x01;
	msgSend[5] = board;
	msgSend[6] = ((reg >> 8) & 0xFF);
	msgSend[7] = (reg & 0xFF);
	msgSend[8] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 9, &msgRecv[0], 6);

	if ((bytesRead > 2) && (msgRecv[0] == START_END_CHAR) && (msgRecv[1] == CAMERA_MSG_RESP))
	{
		int boardRcv = msgRecv[3];
		if (boardRcv == board)
		{
			value = msgRecv[4];

			LOG4CXX_INFO(logger, "Register value is 0x" << std::setfill('0') << std::setw(6) << std::hex << value << " on board " << boardRcv);

			retVal = 0;
		}
	}

	return retVal;
}

int CMicroController::WriteCameraRegister(int board, unsigned int reg, unsigned short value, bool onetime)
{
	int retVal = -1;

	if (m_CameraChipMap[board] == SIMULATED)
	{
		LOG4CXX_INFO(logger, "Ignoring write register on simulated camera board " << board);

		return retVal;
	}

	LOG4CXX_INFO(logger, "Writing register " << std::hex << std::showbase << reg	<< " to " << value << " on board " << std::dec << board << (onetime? " [ONE TIME ONLY]" : ""));
	//LOG4CXX_INFO(logger,printf( "Writing register 0x%x to %d on board %d %s",reg,value, board,(onetime ? " [ONE TIME ONLY]" : "")));

	// 0x7e 0x02 0x06 0x4E 0x01 0x01 0xHH 0xLL 0xVV 0x7e
	byte msgSend[10];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = CAMERA_MSG;
	msgSend[2] = 0x06;
	msgSend[3] = onetime ? SPI_WRITE_CMD_ONE_TIME : SPI_WRITE_CMD;
	msgSend[4] = 0x01;
	msgSend[5] = board;
	msgSend[6] = ((reg >> 8) & 0xFF);
	msgSend[7] = (reg & 0xFF);
	msgSend[8] = value;
	msgSend[9] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 10, &msgRecv[0], 6);

	if ((bytesRead > 2) && (msgRecv[0] == START_END_CHAR) && (msgRecv[1] == 0x12))
	{
		LOG4CXX_DEBUG(logger, "Register was set");
		retVal = 0;
	}

	return retVal;
}

int CMicroController::WriteCameraHighLowRegister(int board, unsigned int highReg,
	unsigned int lowReg,
	unsigned int value)
{
#if 0
	int retVal = WriteCameraRegister(board, highReg, ((value >> 8) & 0xFF));
	retVal |= WriteCameraRegister(board, lowReg, (value & 0xFF));

	return retVal;
#else
	int retVal = -1;

	if (m_CameraChipMap[board] == SIMULATED)
	{
		LOG4CXX_INFO(logger, "Ignoring write register on simulated camera board " << board);

		return retVal;
	}

	unsigned short highValue = ((value >> 8) & 0xFF);
	unsigned short lowValue = (value & 0xFF);

	LOG4CXX_DEBUG(logger, "Writing registers [" << std::hex << std::showbase << highReg << "," 
		<< std::hex << std::showbase << lowReg << "] to [" 
		<< std::hex << std::showbase << highValue << "," 
		<< std::hex << std::showbase << lowValue <<"] on board " << std::dec << board);

	// 0x7e 0x02 0x06 0x4E 0x01 0x01 0xHH 0xLL 0xVV 0x7e
	byte msgSend[11];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = CAMERA_MSG;
	msgSend[2] = 0x07;
	msgSend[3] = SPI_WRITE_CMD;
	msgSend[4] = 0x02;
	msgSend[5] = board;
	msgSend[6] = ((highReg >> 8) & 0xFF);
	msgSend[7] = (highReg & 0xFF);
	msgSend[8] = highValue;
	msgSend[9] = lowValue;
	msgSend[10] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 11, &msgRecv[0], 6);

	if ((bytesRead > 2) && (msgRecv[0] == START_END_CHAR) && (msgRecv[1] == 0x12))
	{
		LOG4CXX_DEBUG(logger, "Register was set");
		retVal = 0;
	}

	return retVal;

#endif
}

int CMicroController::SendCameraInit(int board, bool showError)
{
	int retVal = -1;

	if (m_CameraChipMap[board] == SIMULATED)
	{
		LOG4CXX_INFO(logger, "Ignoring sending camera init to simulated board " << board);

		return retVal;
	}

	LOG4CXX_INFO(logger, "Sending camera init to board " << board)

	// Analog 0x7e 0x02 0x05 0x4a 0x01 0x01 0x01 0x00 0x7e
	// MIPI	0x7e 0x02 0x05 0x4a 0x01 0x05 0x00 0x00 0x7e

	byte msgSend[9];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = CAMERA_MSG;
	msgSend[2] = 0x05;
	msgSend[3] = SPI_INIT_CAMERA_CMD;
	msgSend[4] = 0x1;
	msgSend[5] = board;
	msgSend[6] = (m_CameraChipMap[board] == OV426)? 1:0;
	msgSend[7] = 0;
	msgSend[8] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 9, &msgRecv[0], 5);

	if ((bytesRead > 2) && (msgRecv[0] == START_END_CHAR) && (msgRecv[1] == 0x12))
	{
		LOG4CXX_INFO(logger, "Init value was set");
		retVal = 0;
	}
	else if (_bConnected)
	{
		std::stringstream ss;
		ss << "Error (" 
			<< std::showbase << std::setfill('0') << std::setw(2) << std::hex << (int)msgRecv[1]
			<< ") initializing CAM" << std::dec << board << ": ";
		for (int i = 0; i < msgRecv[2]; i++)
		{
			ss << std::showbase << std::setfill('0') << std::setw(2) << std::hex << (int)((const BYTE*)msgRecv)[i+3] << " ";
		}
		LOG4CXX_ERROR(logger, ss.str());

		if (showError)
		{
			theApp.ShowMessageBox(ss.str().c_str(), "Init Error", MB_OK | MB_TOPMOST);
		}
	}

	if (m_CameraChipMap[board] == OV9734)
	{
		// It Front camera, set flip mode
		WriteCameraRegister(board, 0x3820, 0x18);
	}
	else
	{
		// If side camera, set DVP_EOL_VSYNC_DELAY
		WriteCameraRegister(board, 0x4707, 0xd0);
	}

	if (board == 1)
	{
		LOG4CXX_INFO(logger, "Setting 0x3027 to 0x22 for CAM1");
		WriteCameraRegister(board, 0x3027, 0x22);
	}

	return retVal;
}


int CMicroController::SetLEDValue(int channel, float value)
{
	int retVal = -1;

	byte byteVal = (int)round(value * 255);

	LOG4CXX_DEBUG(logger, "Setting led #" << channel << " to " << (int)byteVal);

	byte msgSend[6];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = SET_LED_LEVEL;
	msgSend[2] = 0x02;
	msgSend[3] = channel;
	msgSend[4] = byteVal;
	msgSend[5] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 6, &msgRecv[0], 6);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == SET_LED_LEVEL_RESP))
	{
		LOG4CXX_DEBUG(logger, "LED value was set");
		retVal = 0;
	}

	return retVal;
}

int CMicroController::SetLEDValues(std::map<int, float> values)
{
	int retVal = -1;

	std::stringstream ss;
	ss << "Setting leds ";

	byte msgSend[64];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = SET_LED_LEVELS;
	msgSend[2] = 2*values.size();

	int index = 3;
	for (std::map<int, float>::iterator it = values.begin(); it != values.end(); ++it)
	{
		byte byteVal = (int)round(it->second * 255);
		msgSend[index++] = it->first;
		msgSend[index++] = byteVal;
	}
	msgSend[index++] = START_END_CHAR;

	LOG4CXX_DEBUG(logger, ss.str());

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], index, &msgRecv[0], 6);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == SET_LED_LEVELS_RESP))
	{
		LOG4CXX_DEBUG(logger, "LED values were set");
		retVal = 0;
	}

	return retVal;
}

int CMicroController::SetLEDLevel(bool front, int level)
{
	int retVal = -1;

	LOG4CXX_INFO(logger, "Setting " << (front ? "Front" : "Side") << " led level to " << level);

	byte msgSend[6];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = TOGGLE_LIGHTS;
	msgSend[2] = 0x02;
	msgSend[3] = front ? 0 : 1;
	msgSend[4] = level;
	msgSend[5] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 6, &msgRecv[0], 6);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == TOGGLE_LIGHTS_RESP))
	{
		LOG4CXX_INFO(logger, (front ? "Front" : "Side") << " led level set to " << level);
		retVal = 0;
	}

	return retVal;
}

int CMicroController::SetShadowCast(int delayMs, int bitmask1, int bitmask2)
{
	int retVal = -1;

	delayMs = std::min(delayMs, (int)0x7FFF); //15 bit max

	LOG4CXX_INFO(logger, "Shadow cast delay " << delayMs << " ms with group1 = " 
							<< std::setfill('0') << std::setw(8) << std::hex << bitmask1 << " and group2 = "
							<< std::setfill('0') << std::setw(8) << std::hex << bitmask2);

	byte msgSend[14];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = SET_SHADOWCAST;
	msgSend[2] = 0x0A;
	msgSend[3] = (delayMs & 0xFF);
	msgSend[4] = ((delayMs >> 8) & 0xFF);
	msgSend[5] = (bitmask1 & 0xFF);
	msgSend[6] = ((bitmask1 >> 8) & 0xFF);
	msgSend[7] = ((bitmask1 >> 16) & 0xFF);
	msgSend[8] = ((bitmask1 >> 24) & 0xFF);
	msgSend[9] = (bitmask2 & 0xFF);
	msgSend[10] = ((bitmask2 >> 8) & 0xFF);
	msgSend[11] = ((bitmask2 >>16) & 0xFF);
	msgSend[12] = ((bitmask2 >> 24) & 0xFF);
	msgSend[13] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 14, &msgRecv[0], 4);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == SET_SHADOWCAST_RESP))
	{
		LOG4CXX_INFO(logger, "Shadow cast mode updated");
		retVal = 0;
	}

	return retVal;
}

int CMicroController::SetLEDMUXPeriod(unsigned int period)
{
	int retVal = -1;

	period = std::min(period, (unsigned int)0xFFFF); //16 bit max

	LOG4CXX_INFO(logger, "LED MUX Period " << period);

	byte msgSend[7];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = TEST_HW_CMD;
	msgSend[2] = 0x03;
	msgSend[3] = 0x80;
	msgSend[4] = ((period >> 8) & 0xFF);
	msgSend[5] = (period & 0xFF);
	msgSend[6] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 7, &msgRecv[0], 5);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == TEST_HW_CMD_RESP))
	{
		LOG4CXX_INFO(logger, "LED MUX Period updated");
		retVal = 0;
	}

	return retVal;
}

void CMicroController::ToggleCameraResetLine(bool bRecovery)
{
	m_CriticalSectionResetCameras.Lock();

	SetCameraResetLine(0x1); // 1=Camera
	if (bRecovery)
	{
		Sleep(200);
		SetCameraResetLine(0x2); // 2=FX3
	}
	Sleep(200);
	SetCameraResetLine(0x0);
	Sleep(5000);

	m_CriticalSectionResetCameras.Unlock();
}

void CMicroController::SetCameraResetLine(byte bits)
{
	// 0x7e 0x09 0x01 0x01 0x7e
	
	byte msgSend[5];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = CAMERA_RESET_OUTPUT;
	msgSend[2] = 0x01;
	msgSend[3] = bits;
	msgSend[4] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 5, &msgRecv[0], 8);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == CAMERA_RESET_OUTPUT_RESP))
	{
		LOG4CXX_INFO(logger, "Reset line was set to " << std::hex << (int)bits);
		//LOG4CXX_INFO(logger, printf("Reset line was set to 0x%x",bits)); // javqui 
	}
}

std::string CMicroController::ReadEEPromSN(EEPromLocation location)
{
	std::string value;
	
	// 0x7e 0x05 0x01 0x00 0x7e

	byte msgSend[5];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = READ_EEPROM;
	msgSend[2] = 0x01;
	msgSend[3] = (location == ProcessorBox ? EEP_RW_BOX_SN : EEP_RW_SCOPE_SN);
	msgSend[4] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 5, &msgRecv[0], 20);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == READ_EEPROM_RESP))
	{
		int len = msgRecv[2] - 1;
		for (int i = 0; i < len; i++)
		{
			char c = (char)msgRecv[4 + i];
			if (c == 0)
			{
				value += "<>";
			}
			else
			{
				value += c;
			}
		}
		LOG4CXX_INFO(logger, "ReadEEProm for location " << (int)msgRecv[3] << " returned: " << value);
	}

	return value;
}

void CMicroController::WriteEEPromSN(EEPromLocation location, std::string value)
{
	// 0x7e 0x05 0x01 0x00 0x7e

	byte chars[16];
	memset(chars, 0, 16);
	int len = std::min(16, (int)value.size());
	for (int i = 0; i < len; i++)
	{
		chars[i] = value[i];
	}

	byte msgSend[21];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = WRITE_EEPROM;
	msgSend[2] = 0x11;
	msgSend[3] = (location == ProcessorBox ? EEP_RW_BOX_SN : EEP_RW_SCOPE_SN);

	for (int i = 0; i < 16; i++)
	{
		msgSend[4 + i] = chars[i];
	}

	msgSend[20] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 21, &msgRecv[0], 5);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == WRITE_EEPROM_RESP))
	{
		LOG4CXX_INFO(logger, "WriteEEProm for location " << (int)location << " complete");

		if (location == Scope)
		{
			int ret = theApp.ShowMessageBox(_T("Do you also want to update the hot swap scope EEPROM register"), _T("Info"), MB_YESNO | MB_TOPMOST);
			if (ret == IDYES)
			{
				WriteEEPromScopeHotSwap();
			}
		}
	}
}

void CMicroController::WriteEEPromScopeHotSwap()
{
	//0x7e 0x06 0x08 0x11 0x02 0xfe 0xff 0x03 0x00 0xA5 0x5A 0x7e

	byte msgSend[12];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = WRITE_EEPROM;
	msgSend[2] = 0x08;
	msgSend[3] = 0x11;
	msgSend[4] = 0x02;
	msgSend[5] = 0xfe;
	msgSend[6] = 0xff;
	msgSend[7] = 0x03;
	msgSend[8] = 0x00;
	msgSend[9] = 0xA5;
	msgSend[10] = 0x5A;
	msgSend[11] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 12, &msgRecv[0], 5);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == WRITE_EEPROM_RESP))
	{
		LOG4CXX_INFO(logger, "WriteEEProm Scope Hot Swap complete");
	}
}


std::string CMicroController::ReadEEProm(EEPromLocation location)
{
	std::string value;

	int retVal = 0;
	int offset = EEP_DATA_LENGTH_LOC;
	const int start = EEP_DATA_START_LOC;
	std::vector<byte> bytes;

	// Get the size of the buffer
	bytes.resize(4);
	retVal = ReadEEPromMemoryLocation(location, offset, bytes);
	if (retVal == 0)
	{
		unsigned int count = 0;
		for (int i = 0; i < 4; i++)
		{
			count += (bytes[i] << (8 * (3 - i)));
		}
		LOG4CXX_INFO(logger, "EEProm in " << (location == EEP_RW_BOX ? "Box" : "Scope") << " has " << count << " bytes");

		if (count > EEP_MAX_ADDR)
		{
			LOG4CXX_ERROR(logger, "Too many bytes to read in EEPROM");
			return std::string();
		}

		value.resize(count);
		offset = start;

		while ((offset - start) < count)
		{
			int length = std::min(PCOM_EEPROM_RW_LIMIT, (int)count - (offset - start));
			bytes.resize(length);
			retVal = ReadEEPromMemoryLocation(location, offset, bytes);
			if (retVal == 0)
			{
				memcpy(&value[offset - start], &bytes[0], length);
				offset += length;
			}
			else
			{
				LOG4CXX_ERROR(logger, "WriteEEPromMemoryLocation failure at location 0x"
					<< std::setfill('0') << std::setw(8) << std::hex << offset << " for " << std::dec << length << " bytes");
				break;
			}
		} 
	}
	else
	{
		LOG4CXX_ERROR(logger, "ReadEEPromMemoryLocation failure to get buffer size");
	}

	return value;
}

int CMicroController::WriteEEProm(EEPromLocation location, std::string value)
{
	int retVal = -1;
	int offset = EEP_DATA_LENGTH_LOC;
	const int start = EEP_DATA_START_LOC;
	std::vector<byte> bytes;

	unsigned int count = value.size();

	LOG4CXX_INFO(logger, "Writing EEProm in " << (location == EEP_RW_BOX ? "Box" : "Scope") << " with " << count << " bytes");

	if (count > EEP_MAX_ADDR)
	{
		count = EEP_MAX_ADDR;
		LOG4CXX_ERROR(logger, "Limiting EEPROM write to " << count << " bytes");
	}

	// Write the size of the buffer
	bytes.resize(4);
	for (int i = 0; i < 4; i++)
	{
		bytes[i] = ((count >> (8 * (3 - i))) & 0xFF);
	}
	retVal = WriteEEPromMemoryLocation(location, offset, bytes);
	if (retVal == 0)
	{
		offset = start;

		while ((offset - start) < count)
		{
			int length = std::min(PCOM_EEPROM_RW_LIMIT, (int)count - (offset - start));
			bytes.resize(length);
			memcpy(&bytes[0], &value[offset - start], length);
			retVal = WriteEEPromMemoryLocation(location, offset, bytes);
			if (retVal == 0)
			{
				offset += length;
			}
			else
			{
				LOG4CXX_ERROR(logger, "WriteEEPromMemoryLocation failure at location 0x" 
					<< std::setfill('0') << std::setw(8) << std::hex << offset << " for " << std::dec << length << " bytes");
				break;
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(logger, "WriteEEPromMemoryLocation failure to set buffer size");
	}

	return retVal;
}

int CMicroController::ReadEEPromMemoryLocation(EEPromLocation location, unsigned int offset, std::vector<byte> &bytes)
{
	int retVal = -1;

	LOG4CXX_INFO(logger, "Reading  " << (location == ProcessorBox ? "Box" : "Scope") << " EEProm at location 0x" << std::setfill('0') << std::setw(8) << std::hex << offset << " byte count " << std::dec << bytes.size());
	//LOG4CXX_INFO(logger, printf("Reading  %s EEProm at location:0x%x byte count:%d",(location == ProcessorBox ? "Box" : "Scope"),offset,bytes.size() ) ); // javqui
	//Read from scope 2  bytes at addr 0x010000
	//0x7e 0x05 0x06 0x11 0x02 0x00 0x00 0x01 0x00 0x7e

	byte msgSend[10];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = READ_EEPROM;
	msgSend[2] = 0x06;
	msgSend[3] = (location == ProcessorBox ? EEP_RW_BOX : EEP_RW_SCOPE);
	msgSend[4] = bytes.size();
#if 0
	msgSend[5] = ((offset >> 24) & 0xFF);
	msgSend[6] = ((offset >> 16) & 0xFF);
	msgSend[7] = ((offset >> 8) & 0xFF);
	msgSend[8] = (offset & 0xFF);
#else
	msgSend[5] = (offset & 0xFF);
	msgSend[6] = ((offset >> 8) & 0xFF);
	msgSend[7] = ((offset >> 16) & 0xFF);
	msgSend[8] = ((offset >> 24) & 0xFF);
#endif
	msgSend[9] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 10, &msgRecv[0], 5 + bytes.size());

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == READ_EEPROM_RESP))	{
		int len = msgRecv[2] - 1;
		if (len = bytes.size())	{
			for (int i = 0; i < len; i++){
				bytes[i] = msgRecv[4 + i];
			}
			retVal = 0;

			LOG4CXX_INFO(logger, "ReadEEProm  for location 0x" << std::setfill('0') << std::setw(8) << std::hex << offset << " complete");
			//LOG4CXX_INFO(logger, printf("ReadEEProm  for location 0x%x complete", offset));
		} else{
			LOG4CXX_ERROR(logger, "ReadEEProm expected " << bytes.size() << " bytes, but received " << len);
		}
	}

	return retVal;
}

int CMicroController::WriteEEPromMemoryLocation(EEPromLocation location, unsigned int offset, std::vector<byte> &bytes)
{
	int retVal = -1;

	if (bytes.size() > PCOM_EEPROM_RW_LIMIT)
	{
		LOG4CXX_INFO(logger, "Unable to write more than " << PCOM_EEPROM_RW_LIMIT << " bytes to EEProm at a time");
		return retVal;
	}

	LOG4CXX_INFO(logger, "Writing  " << (location == ProcessorBox ? "Box" : "Scope") << " EEProm at location 0x"
		<< std::setfill('0') << std::setw(8) << std::hex << offset << " byte count " << std::dec << bytes.size());

	// Write to scope 1 data byte at addr  0x010000
	// 0x7e 0x06 0x07 0x11 0x01 0x00 0x00 0x01 0x00 0xa5 0x7e

	byte msgSend[128];
	msgSend[0] = START_END_CHAR;
	msgSend[1] = WRITE_EEPROM;
	msgSend[2] = 6 + bytes.size();
	msgSend[3] = (location == ProcessorBox ? EEP_RW_BOX : EEP_RW_SCOPE);
	msgSend[4] = bytes.size();
#if 0
	msgSend[5] = ((offset >> 24) & 0xFF);
	msgSend[6] = ((offset >> 16) & 0xFF);
	msgSend[7] = ((offset >> 8) & 0xFF);
	msgSend[8] = (offset & 0xFF);
#else
	msgSend[5] = (offset & 0xFF);
	msgSend[6] = ((offset >> 8) & 0xFF);
	msgSend[7] = ((offset >> 16) & 0xFF);
	msgSend[8] = ((offset >> 24) & 0xFF);
#endif
	for (int i = 0; i < bytes.size(); i++)
	{
		msgSend[9 + i] = bytes[i];
	}
	msgSend[9 + bytes.size()] = START_END_CHAR;

	byte msgRecv[128];
	DWORD bytesRead = SendMesage(&msgSend[0], 10 + bytes.size(), &msgRecv[0], 5);

	if ((msgRecv[0] == START_END_CHAR) && (msgRecv[1] == WRITE_EEPROM_RESP))
	{
		retVal = 0;

		LOG4CXX_INFO(logger, "WriteEEProm for location 0x"
			<< std::setfill('0') << std::setw(8) << std::hex << offset << " complete");
	}

	// Need to give some time for write to make it to EEPROM
	// With fast successive message, some block are not written before next block
	Sleep(10);

	return retVal;
}

std::vector<byte> CMicroController::PreprocessForSend(const void* lpSendBuf, DWORD dwSendCount)
{
	std::vector<byte> output;
	output.reserve(dwSendCount);
	
	const byte *it = (const byte *)lpSendBuf;
	output.push_back(*it++); // first '7e'
	for (int i = 1; i < dwSendCount - 1; i++, it++)
	{
		switch (*it)
		{
		case 0x7e:
			output.push_back(0x7d);
			output.push_back(0x5e);
			break;
		case 0x7d:
			output.push_back(0x7d);
			output.push_back(0x5d);
			break;
		default:
			output.push_back(*it);
			break;
		}
	}
	output.push_back(*it); // last '7e'

	return output;
}

std::vector<byte> CMicroController::PreprocessFromReceive(const void* lpRecvBuf, DWORD dwRecvCount)
{
	std::vector<byte> output;
	output.reserve(dwRecvCount);

	const byte *it = (const byte *)lpRecvBuf;
	for (int i = 0; i < dwRecvCount; i++, it++)
	{
		switch (*it)
		{
		case 0x7d:
			++it;
			if (*it == 0x5d)
				output.push_back(0x7d);
			else if (*it == 0x5e)
				output.push_back(0x7e);
			break;
		default:
			output.push_back(*it);
			break;
		}
	}

	return output;

}

int CMicroController::ToggleUSBDeviceState()
{
	int retVal = FALSE;

	LOG4CXX_DEBUG(logger, "ToggleUSBDeviceState");

	unsigned i;
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	WCHAR szBuffer[4096];
	TCHAR szDesc[1024], szDeviceInstanceID[MAX_DEVICE_ID_LEN];
	DEVPROPTYPE ulPropertyType;
	DWORD dwSize, dwPropertyRegDataType;
	FN_SetupDiGetDevicePropertyW fn_SetupDiGetDevicePropertyW = (FN_SetupDiGetDevicePropertyW)
		GetProcAddress(GetModuleHandle(TEXT("Setupapi.dll")), "SetupDiGetDevicePropertyW");

	// List all connected USB devices
	hDevInfo = SetupDiGetClassDevs(NULL, TEXT("USB"), NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
		return retVal;

	// Loop through all the devices to find the match
	for (i = 0;; i++)
	{
		DeviceInfoData.cbSize = sizeof(DeviceInfoData);
		if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData))
			break;

		CONFIGRET status = CM_Get_Device_ID(DeviceInfoData.DevInst, szDeviceInstanceID, MAX_PATH, 0);
		if (status != CR_SUCCESS)
			continue;

		if (SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC,
			&dwPropertyRegDataType, (BYTE*)szDesc, sizeof(szDesc), &dwSize))
		{
			if (wcsstr(szDesc, USB_DESC) == 0)
				continue;
		}

		// Retreive the Manufacturer device description as reported by the device itself
		if (fn_SetupDiGetDevicePropertyW && fn_SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_Manufacturer,
			&ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0))
		{
			if (wcsstr(szBuffer, USB_MANUFACTURER) == 0)
				continue;
		}

		LOG4CXX_DEBUG(logger, "Found device: " << CString(szDeviceInstanceID));

		SP_DEVINSTALL_PARAMS devParams;
		devParams.cbSize = sizeof(devParams);
		SetupDiGetDeviceInstallParams(hDevInfo, &DeviceInfoData, &devParams);

		SP_PROPCHANGE_PARAMS pcp;
		pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
		pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
		pcp.StateChange = DICS_DISABLE;
		pcp.Scope = DICS_FLAG_GLOBAL;
		pcp.HwProfile = 0;
		if (SetupDiSetClassInstallParams(hDevInfo, &DeviceInfoData, &pcp.ClassInstallHeader, sizeof(pcp)))
		{
			retVal = SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, &DeviceInfoData);
			if (retVal)
			{
				LOG4CXX_DEBUG(logger, "Device disabled");
			}
			else
			{
				LOG4CXX_ERROR(logger, "Failed to disable device: " << GetLastError());
			}
		}
		else
		{
			LOG4CXX_ERROR(logger, "Failed set disable settings: " << GetLastError());
		}


		pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
		pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
		pcp.StateChange = DICS_ENABLE;
		pcp.Scope = DICS_FLAG_GLOBAL;
		pcp.HwProfile = 0;
		if (SetupDiSetClassInstallParams(hDevInfo, &DeviceInfoData, &pcp.ClassInstallHeader, sizeof(pcp)))
		{
			retVal = SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hDevInfo, &DeviceInfoData);

			if (retVal)
			{
				LOG4CXX_DEBUG(logger, "Device enabled");
			}
			else
			{
				LOG4CXX_ERROR(logger, "Failed to enable device: " << GetLastError());
			}
		}
		else
		{
			LOG4CXX_ERROR(logger, "Failed set enable settings: " << GetLastError());
		}
	}

	LOG4CXX_DEBUG(logger, "ToggleUSBDeviceState Done");

	return retVal;
}