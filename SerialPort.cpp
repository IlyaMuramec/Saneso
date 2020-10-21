/*
Module : SERIALPORT.CPP
Purpose: Implementation for an MFC wrapper class for serial ports
Created: PJN / 31-05-1999
History: PJN / 03-06-1999 1. Fixed problem with code using CancelIo which does not exist on 95.
                          2. Fixed leaks which can occur in sample app when an exception is thrown
         PJN / 16-06-1999 1. Fixed a bug whereby CString::ReleaseBuffer was not being called in 
                             CSerialException::GetErrorMessage
         PJN / 29-09-1999 1. Fixed a simple copy and paste bug in CSerialPort::SetDTR

Copyright (c) 1999 by PJ Naughter.  
All rights reserved.

*/

/////////////////////////////////  Includes  //////////////////////////////////
#include "stdafx.h"
#include "serialport.h"
#include "winerror.h"

#pragma warning(disable : 4996)


///////////////////////////////// defines /////////////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////// Implementation ///////////////////////////////

log4cxx::LoggerPtr CSerialPort::logger = log4cxx::Logger::getLogger("SerialPort");

//Class which handles CancelIo function which must be constructed at run time
//since it is not imeplemented on NT 3.51 or Windows 95. To avoid the loader
//bringing up a message such as "Failed to load due to missing export...", the
//function is constructed using GetProcAddress. The CSerialPort::CancelIo 
//function then checks to see if the function pointer is NULL and if it is it 
//throws an exception using the error code ERROR_CALL_NOT_IMPLEMENTED which
//is what 95 would have done if it had implemented a stub for it in the first
//place !!

class _SERIAL_PORT_DATA
{
public:
//Constructors /Destructors
  _SERIAL_PORT_DATA();
  ~_SERIAL_PORT_DATA();

  HINSTANCE m_hKernel32;
  typedef BOOL (CANCELIO)(HANDLE);
  typedef CANCELIO* LPCANCELIO;
  LPCANCELIO m_lpfnCancelIo;
};

_SERIAL_PORT_DATA::_SERIAL_PORT_DATA()
{
  m_hKernel32 = LoadLibrary(_T("KERNEL32.DLL"));
  VERIFY(m_hKernel32 != NULL);
  m_lpfnCancelIo = (LPCANCELIO) GetProcAddress(m_hKernel32, "CancelIo");
}

_SERIAL_PORT_DATA::~_SERIAL_PORT_DATA()
{
  FreeLibrary(m_hKernel32);
  m_hKernel32 = NULL;
}


//The local variable which handle the function pointers

_SERIAL_PORT_DATA _SerialPortData;




// **************************************************************************************************************8
// Exception handling code
// **************************************************************************************************************8

void AfxThrowSerialException(DWORD dwError /* = 0 */)
{
	if (dwError == 0)
		dwError = ::GetLastError();

	CSerialException* pException = new CSerialException(dwError);

	LOG4CXX_ERROR(CSerialPort::logger, "Warning: throwing CSerialException for error " << dwError);
	THROW(pException);
}

BOOL CSerialException::GetErrorMessage(LPTSTR pstrError, UINT nMaxError, PUINT pnHelpContext)
{
	ASSERT(pstrError != NULL && AfxIsValidString(pstrError, nMaxError));

	if (pnHelpContext != NULL)
		*pnHelpContext = 0;

	LPTSTR lpBuffer;
	BOOL bRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			                      NULL,  m_dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
			                      (LPTSTR) &lpBuffer, 0, NULL);

	if (bRet == FALSE)
		*pstrError = '\0';
	else
	{
		lstrcpyn(pstrError, lpBuffer, nMaxError);
		bRet = TRUE;

		LocalFree(lpBuffer);
	}

	return bRet;
}

CString CSerialException::GetErrorMessage()
{
  CString rVal;
  LPTSTR pstrError = rVal.GetBuffer(4096);
  GetErrorMessage(pstrError, 4096, NULL);
  rVal.ReleaseBuffer();
  return rVal;
}

CSerialException::CSerialException(DWORD dwError)
{
	m_dwError = dwError;
}

CSerialException::~CSerialException()
{
}

IMPLEMENT_DYNAMIC(CSerialException, CException)

#ifdef _DEBUG
void CSerialException::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);

	dc << "m_dwError = " << m_dwError;
}
#endif




// **************************************************************************************************************8
// The actual serial port code
// **************************************************************************************************************8
CSerialPort::CSerialPort()
{
  m_hComm = INVALID_HANDLE_VALUE;
  m_bOverlapped = FALSE;
  m_nComPort = -1;
}

CSerialPort::~CSerialPort()
{
  Close();
}

IMPLEMENT_DYNAMIC(CSerialPort, CObject)

#ifdef _DEBUG
void CSerialPort::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);

	dc << _T("m_hComm = ") << m_hComm << _T("\n");
  dc << _T("m_bOverlapped = ") << m_bOverlapped;
}
#endif

void CSerialPort::Open(int nPort, DWORD dwBaud, Parity parity, BYTE DataBits, StopBits stopbits, FlowControl fc, BOOL bOverlapped)
{
	//Validate our parameters
	ASSERT(nPort>0 && nPort<=255);

	m_nComPort = nPort;

	//Call CreateFile to open up the comms port
	CString sPort;
	sPort.Format(_T("\\\\.\\COM%d"), nPort);
	m_hComm = CreateFile(sPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, bOverlapped ? FILE_FLAG_OVERLAPPED : 0, NULL);
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LOG4CXX_ERROR(logger, "Failed to open up the comms port");
		return;
	}

	m_bOverlapped = bOverlapped;

	//Get the current state prior to changing it
	DCB dcb;
	GetState(dcb);

	//Setup the baud rate
	dcb.BaudRate = dwBaud; 

	//Setup the Parity
	switch (parity)
	{
	case EvenParity:  dcb.Parity = EVENPARITY;  break;
	case MarkParity:  dcb.Parity = MARKPARITY;  break;
	case NoParity:    dcb.Parity = NOPARITY;    break;
	case OddParity:   dcb.Parity = ODDPARITY;   break;
	case SpaceParity: dcb.Parity = SPACEPARITY; break;
	default:          ASSERT(FALSE);            break;
	}

	//Setup the data bits
	dcb.ByteSize = DataBits;

	//Setup the stop bits
	switch (stopbits)
	{
	case OneStopBit:           dcb.StopBits = ONESTOPBIT;   break;
	case OnePointFiveStopBits: dcb.StopBits = ONE5STOPBITS; break;
	case TwoStopBits:          dcb.StopBits = TWOSTOPBITS;  break;
	default:                   ASSERT(FALSE);               break;
	}

	//Setup the flow control 
	dcb.fDsrSensitivity = FALSE;
	switch (fc)
	{
	case NoFlowControl:
	{
	  dcb.fOutxCtsFlow = FALSE;
	  dcb.fOutxDsrFlow = FALSE;
	  dcb.fOutX = FALSE;
	  dcb.fInX = FALSE;
	  break;
	}
	case CtsRtsFlowControl:
	{
	  dcb.fOutxCtsFlow = TRUE;
	  dcb.fOutxDsrFlow = FALSE;
	  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	  dcb.fOutX = FALSE;
	  dcb.fInX = FALSE;
	  break;
	}
	case CtsDtrFlowControl:
	{
	  dcb.fOutxCtsFlow = TRUE;
	  dcb.fOutxDsrFlow = FALSE;
	  dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
	  dcb.fOutX = FALSE;
	  dcb.fInX = FALSE;
	  break;
	}
	case DsrRtsFlowControl:
	{
	  dcb.fOutxCtsFlow = FALSE;
	  dcb.fOutxDsrFlow = TRUE;
	  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	  dcb.fOutX = FALSE;
	  dcb.fInX = FALSE;
	  break;
	}
	case DsrDtrFlowControl:
	{
	  dcb.fOutxCtsFlow = FALSE;
	  dcb.fOutxDsrFlow = TRUE;
	  dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
	  dcb.fOutX = FALSE;
	  dcb.fInX = FALSE;
	  break;
	}
	case XonXoffFlowControl:
	{
	  dcb.fOutxCtsFlow = FALSE;
	  dcb.fOutxDsrFlow = FALSE;
	  dcb.fOutX = TRUE;
	  dcb.fInX = TRUE;
	  dcb.XonChar = 0x11;
	  dcb.XoffChar = 0x13;
	  dcb.XoffLim = 100;
	  dcb.XonLim = 100;
	  break;
	}
	default:
	{
	  ASSERT(FALSE);
	  break;
	}
	}

	//Now that we have all the settings in place, make the changes
	SetState(dcb);
}

void CSerialPort::Close()
{
  if (IsOpen())
  {
    BOOL bSuccess = CloseHandle(m_hComm);
    m_hComm = INVALID_HANDLE_VALUE;
    if (!bSuccess)
      LOG4CXX_ERROR(logger, "Failed to close up the comms port, GetLastError: " << GetLastError());
    m_bOverlapped = FALSE;
  }
}

void CSerialPort::Attach(HANDLE hComm)
{
  Close();
  m_hComm = hComm;  
}

HANDLE CSerialPort::Detach()
{
  HANDLE hrVal = m_hComm;
  m_hComm = INVALID_HANDLE_VALUE;
  return hrVal;
}

DWORD CSerialPort::Read(void* lpBuf, DWORD dwCount)
{
  ASSERT(IsOpen());
  ASSERT(!m_bOverlapped);

  DWORD dwBytesRead = 0;
  if (!ReadFile(m_hComm, lpBuf, dwCount, &dwBytesRead, NULL))
  {
    LOG4CXX_ERROR(logger, "Failed in call to ReadFile");
    AfxThrowSerialException();
  }

  return dwBytesRead;
}

BOOL CSerialPort::Read(void* lpBuf, DWORD dwCount, OVERLAPPED& overlapped)
{
  ASSERT(IsOpen());
  ASSERT(m_bOverlapped);
  ASSERT(overlapped.hEvent);

  DWORD dwBytesRead = 0;
  BOOL bSuccess = ReadFile(m_hComm, lpBuf, dwCount, &dwBytesRead, &overlapped);
  if (!bSuccess)
  {
    if (GetLastError() != ERROR_IO_PENDING)
    {
      LOG4CXX_ERROR(logger, "Failed in call to ReadFile");
      AfxThrowSerialException();
    }
  }
  return bSuccess;
}

DWORD CSerialPort::Write(const void* lpBuf, DWORD dwCount)
{
  ASSERT(IsOpen());
  ASSERT(!m_bOverlapped);

  DWORD dwBytesWritten = 0;
  if (!WriteFile(m_hComm, lpBuf, dwCount, &dwBytesWritten, NULL))
  {
    LOG4CXX_ERROR(logger, "Failed in call to WriteFile");
    AfxThrowSerialException();
  }

  return dwBytesWritten;
}

BOOL CSerialPort::Write(const void* lpBuf, DWORD dwCount, OVERLAPPED& overlapped)
{
  ASSERT(IsOpen());
  ASSERT(m_bOverlapped);
  ASSERT(overlapped.hEvent);

  DWORD dwBytesWritten = 0;
  BOOL bSuccess = WriteFile(m_hComm, lpBuf, dwCount, &dwBytesWritten, &overlapped);
  if (!bSuccess)
  {
    if (GetLastError() != ERROR_IO_PENDING)
    {
      LOG4CXX_ERROR(logger, "Failed in call to overlapped WriteFile");
      AfxThrowSerialException();
    }
  }
  return bSuccess;
}

void CSerialPort::GetOverlappedResult(OVERLAPPED& overlapped, DWORD& dwBytesTransferred, BOOL bWait)
{
  ASSERT(IsOpen());
  ASSERT(m_bOverlapped);
  ASSERT(overlapped.hEvent);
  
  if (!::GetOverlappedResult(m_hComm, &overlapped, &dwBytesTransferred, bWait))
  {
    if (GetLastError() != ERROR_IO_PENDING)
    {
      LOG4CXX_ERROR(logger, "Failed in call to GetOverlappedResult");
      AfxThrowSerialException();
    }
  }
}

void CSerialPort::_OnCompletion(DWORD dwErrorCode, DWORD dwCount, LPOVERLAPPED lpOverlapped)
{
  //Validate our parameters
  ASSERT(lpOverlapped);

  //Convert back to the C++ world
  CSerialPort* pSerialPort = (CSerialPort*) lpOverlapped->hEvent;
  ASSERT(pSerialPort->IsKindOf(RUNTIME_CLASS(CSerialPort)));

  //Call the C++ function
  pSerialPort->OnCompletion(dwErrorCode, dwCount, lpOverlapped);
}

void CSerialPort::OnCompletion(DWORD /*dwErrorCode*/, DWORD /*dwCount*/, LPOVERLAPPED lpOverlapped)
{
  //Just free up the memory which was previously allocated for the OVERLAPPED structure
  delete lpOverlapped;

  //Your derived classes can do something useful in OnCompletion, but don't forget to
  //call CSerialPort::OnCompletion to ensure the memory is freed up
}

void CSerialPort::CancelIo()
{
  ASSERT(IsOpen());

  if (_SerialPortData.m_lpfnCancelIo == NULL)
  {
    LOG4CXX_ERROR(logger, "CancelIo function is not supported on this OS. You need to be running at least NT 4 or Win 98 to use this function");
    AfxThrowSerialException(ERROR_CALL_NOT_IMPLEMENTED);  
  }

  if (!::_SerialPortData.m_lpfnCancelIo(m_hComm))
  {
    LOG4CXX_ERROR(logger, "Failed in call to CancelIO");
    AfxThrowSerialException();
  }
}

void CSerialPort::WriteEx(const void* lpBuf, DWORD dwCount)
{
  ASSERT(IsOpen());

  OVERLAPPED* pOverlapped = new OVERLAPPED;
  ZeroMemory(pOverlapped, sizeof(OVERLAPPED));
  pOverlapped->hEvent = (HANDLE) this;
  if (!WriteFileEx(m_hComm, lpBuf, dwCount, pOverlapped, _OnCompletion))
  {
    delete pOverlapped;
    LOG4CXX_ERROR(logger, "Failed in call to WriteFileEx");
    AfxThrowSerialException();
  }
}

void CSerialPort::ReadEx(void* lpBuf, DWORD dwCount)
{
  ASSERT(IsOpen());

  OVERLAPPED* pOverlapped = new OVERLAPPED;
  ZeroMemory(pOverlapped, sizeof(OVERLAPPED));
  pOverlapped->hEvent = (HANDLE) this;
  if (!ReadFileEx(m_hComm, lpBuf, dwCount, pOverlapped, _OnCompletion))
  {
    delete pOverlapped;
    LOG4CXX_ERROR(logger, "Failed in call to ReadFileEx");
    AfxThrowSerialException();
  }
}

void CSerialPort::TransmitChar(char cChar)
{
  ASSERT(IsOpen());

  if (!TransmitCommChar(m_hComm, cChar))
  {
    LOG4CXX_ERROR(logger, "Failed in call to TransmitCommChar");
    AfxThrowSerialException();
  }
}

void CSerialPort::GetConfig(COMMCONFIG& config)
{
  ASSERT(IsOpen());

  DWORD dwSize = sizeof(COMMCONFIG);
  if (!GetCommConfig(m_hComm, &config, &dwSize))
  {
    LOG4CXX_ERROR(logger, "Failed in call to GetCommConfig");
    AfxThrowSerialException();
  }
}

void CSerialPort::SetConfig(COMMCONFIG& config)
{
  ASSERT(IsOpen());

  DWORD dwSize = sizeof(COMMCONFIG);
  if (!SetCommConfig(m_hComm, &config, dwSize))
  {
    LOG4CXX_ERROR(logger, "Failed in call to SetCommConfig");
    AfxThrowSerialException();
  }
}

void CSerialPort::SetBreak()
{
  ASSERT(IsOpen());

  if (!SetCommBreak(m_hComm))
  {
    LOG4CXX_ERROR(logger, "Failed in call to SetCommBreak");
    AfxThrowSerialException();
  }
}

void CSerialPort::ClearBreak()
{
  ASSERT(IsOpen());

  if (!ClearCommBreak(m_hComm))
  {
    LOG4CXX_ERROR(logger, "Failed in call to SetCommBreak");
    AfxThrowSerialException();
  }
}

void CSerialPort::ClearError(DWORD& dwErrors)
{
  ASSERT(IsOpen());

  if (!ClearCommError(m_hComm, &dwErrors, NULL))
  {
    LOG4CXX_ERROR(logger, "Failed in call to ClearCommError");
    AfxThrowSerialException();
  }
}

void CSerialPort::GetDefaultConfig(int nPort, COMMCONFIG& config)
{
  //Validate our parameters
  ASSERT(nPort>0 && nPort<=255);

  //Create the device name as a string
  CString sPort;
  sPort.Format(_T("COM%d"), nPort);

  DWORD dwSize = sizeof(COMMCONFIG);
  if (!GetDefaultCommConfig(sPort, &config, &dwSize))
  {
    LOG4CXX_ERROR(logger, "Failed in call to GetDefaultCommConfig");
    AfxThrowSerialException();
  }
}

void CSerialPort::SetDefaultConfig(int nPort, COMMCONFIG& config)
{
  //Validate our parameters
  ASSERT(nPort>0 && nPort<=255);

  //Create the device name as a string
  CString sPort;
  sPort.Format(_T("COM%d"), nPort);

  DWORD dwSize = sizeof(COMMCONFIG);
  if (!SetDefaultCommConfig(sPort, &config, dwSize))
  {
    LOG4CXX_ERROR(logger, "Failed in call to GetDefaultCommConfig");
    AfxThrowSerialException();
  }
}

void CSerialPort::GetStatus(COMSTAT& stat)
{
  ASSERT(IsOpen());

  DWORD dwErrors;
  if (!ClearCommError(m_hComm, &dwErrors, &stat))
  {
    LOG4CXX_ERROR(logger, "Failed in call to ClearCommError");
    AfxThrowSerialException();
  }
}

void CSerialPort::GetState(DCB& dcb)
{
  ASSERT(IsOpen());

  if (!GetCommState(m_hComm, &dcb))
  {
    LOG4CXX_ERROR(logger, "Failed in call to GetCommState");
    AfxThrowSerialException();
  }
}

void CSerialPort::SetState(DCB& dcb)
{
  ASSERT(IsOpen());

  if (!SetCommState(m_hComm, &dcb))
  {
    LOG4CXX_ERROR(logger, "Failed in call to SetCommState");
    AfxThrowSerialException();
  }
}

void CSerialPort::Escape(DWORD dwFunc)
{
  ASSERT(IsOpen());

  if (!EscapeCommFunction(m_hComm, dwFunc))
  {
    LOG4CXX_ERROR(logger, "Failed in call to EscapeCommFunction");
    AfxThrowSerialException();
  }
}

void CSerialPort::ClearDTR()
{
  Escape(CLRDTR);
}

void CSerialPort::ClearRTS()
{
  Escape(CLRRTS);
}

void CSerialPort::SetDTR()
{
  Escape(SETDTR);
}

void CSerialPort::SetRTS()
{
  Escape(SETRTS);
}

void CSerialPort::SetXOFF()
{
  Escape(SETXOFF);
}

void CSerialPort::SetXON()
{
  Escape(SETXON);
}

void CSerialPort::GetProperties(COMMPROP& properties)
{
  ASSERT(IsOpen());

  if (!GetCommProperties(m_hComm, &properties))
  {
    LOG4CXX_ERROR(logger, "Failed in call to GetCommProperties");
    AfxThrowSerialException();
  }
}

void CSerialPort::GetModemStatus(DWORD& dwModemStatus)
{
  ASSERT(IsOpen());

  if (!GetCommModemStatus(m_hComm, &dwModemStatus))
  {
    LOG4CXX_ERROR(logger, "Failed in call to GetCommModemStatus");
    AfxThrowSerialException();
  }
}

void CSerialPort::SetMask(DWORD dwMask)
{
  ASSERT(IsOpen());

  if (!SetCommMask(m_hComm, dwMask))
  {
    LOG4CXX_ERROR(logger, "Failed in call to SetCommMask");
    AfxThrowSerialException();
  }
}

void CSerialPort::GetMask(DWORD& dwMask)
{
  ASSERT(IsOpen());

  if (!GetCommMask(m_hComm, &dwMask))
  {
    LOG4CXX_ERROR(logger, "Failed in call to GetCommMask");
    AfxThrowSerialException();
  }
}

void CSerialPort::Flush()
{
  ASSERT(IsOpen());

  if (!FlushFileBuffers(m_hComm))
  {
    LOG4CXX_ERROR(logger, "Failed in call to FlushFileBuffers");
    AfxThrowSerialException();
  }
}

void CSerialPort::Purge(DWORD dwFlags)
{
  ASSERT(IsOpen());

  if (!PurgeComm(m_hComm, dwFlags))
  {
    LOG4CXX_ERROR(logger, "Failed in call to PurgeComm");
    AfxThrowSerialException();
  }
}

void CSerialPort::TerminateOutstandingWrites()
{
  Purge(PURGE_TXABORT);
}

void CSerialPort::TerminateOutstandingReads()
{
  Purge(PURGE_RXABORT);
}

void CSerialPort::ClearWriteBuffer()
{
  Purge(PURGE_TXCLEAR);
}

void CSerialPort::ClearReadBuffer()
{
  Purge(PURGE_RXCLEAR);
}

void CSerialPort::Setup(DWORD dwInQueue, DWORD dwOutQueue)
{
  ASSERT(IsOpen());

  if (!SetupComm(m_hComm, dwInQueue, dwOutQueue))
  {
    LOG4CXX_ERROR(logger, "Failed in call to SetupComm");
    AfxThrowSerialException();
  }
}

void CSerialPort::SetTimeouts(COMMTIMEOUTS& timeouts)
{
  ASSERT(IsOpen());

  if (!SetCommTimeouts(m_hComm, &timeouts))
  {
    LOG4CXX_ERROR(logger, "Failed in call to SetCommTimeouts");
    AfxThrowSerialException();
  }
}

void CSerialPort::GetTimeouts(COMMTIMEOUTS& timeouts)
{
  ASSERT(IsOpen());

  if (!GetCommTimeouts(m_hComm, &timeouts))
  {
    LOG4CXX_ERROR(logger, "Failed in call to GetCommTimeouts");
    AfxThrowSerialException();
  }
}

void CSerialPort::Set0Timeout()
{
  COMMTIMEOUTS Timeouts;
  ZeroMemory(&Timeouts, sizeof(COMMTIMEOUTS));
  Timeouts.ReadIntervalTimeout = MAXDWORD;
  Timeouts.ReadTotalTimeoutMultiplier = 0;
  Timeouts.ReadTotalTimeoutConstant = 0;
  Timeouts.WriteTotalTimeoutMultiplier = 0;
  Timeouts.WriteTotalTimeoutConstant = 0;
  SetTimeouts(Timeouts);
}

void CSerialPort::Set0WriteTimeout()
{
  COMMTIMEOUTS Timeouts;
  GetTimeouts(Timeouts);
  Timeouts.WriteTotalTimeoutMultiplier = 0;
  Timeouts.WriteTotalTimeoutConstant = 0;
  SetTimeouts(Timeouts);
}

void CSerialPort::Set0ReadTimeout()
{
  COMMTIMEOUTS Timeouts;
  GetTimeouts(Timeouts);
  Timeouts.ReadIntervalTimeout = MAXDWORD;
  Timeouts.ReadTotalTimeoutMultiplier = 0;
  Timeouts.ReadTotalTimeoutConstant = 0;
  SetTimeouts(Timeouts);
}

void CSerialPort::WaitEvent(DWORD& dwMask)
{
  ASSERT(IsOpen());
  ASSERT(!m_bOverlapped);

  if (!WaitCommEvent(m_hComm, &dwMask, NULL))
  {
    LOG4CXX_ERROR(logger, "Failed in call to WaitCommEvent");
    AfxThrowSerialException();
  }
}

void CSerialPort::WaitEvent(DWORD& dwMask, OVERLAPPED& overlapped)
{
  ASSERT(IsOpen());
  ASSERT(m_bOverlapped);
  ASSERT(overlapped.hEvent);

  if (!WaitCommEvent(m_hComm, &dwMask, &overlapped))
  {
    if (GetLastError() != ERROR_IO_PENDING)
    {
      LOG4CXX_ERROR(logger, "Failed in call to WaitCommEvent");
      AfxThrowSerialException();
    }
  }
}

void CSerialPort::EnumerateSerialPorts(CUIntArray &ports)
{
  //Make sure we clear out any elements which may already be in the array
  ports.RemoveAll();

  //Determine what OS we are running on
  OSVERSIONINFO osvi;
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  BOOL bGetVer = GetVersionEx(&osvi);

  //On NT use the QueryDosDevice API
  if (bGetVer && (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT))
  {
    //Use QueryDosDevice to look for all devices of the form COMx. This is a better
    //solution as it means that no ports have to be opened at all.
    TCHAR szDevices[65535];
    DWORD dwChars = QueryDosDevice(NULL, szDevices, 65535);
    if (dwChars)
    {
      int i=0;

      for (;;)
      {
        //Get the current device name
        TCHAR* pszCurrentDevice = &szDevices[i];

        //If it looks like "COMX" then
        //add it to the array which will be returned
        int nLen = (int) _tcslen(pszCurrentDevice);
        if (nLen > 3 && _tcsnicmp(pszCurrentDevice, _T("COM"), 3) == 0)
        {
          //Work out the port number
          int nPort = _ttoi(&pszCurrentDevice[3]);
          ports.Add(nPort);
        }

        // Go to next NULL character
        while(szDevices[i] != _T('\0'))
          i++;

        // Bump pointer to the next string
        i++;

        // The list is double-NULL terminated, so if the character is
        // now NULL, we're at the end
        if (szDevices[i] == _T('\0'))
          break;
      }
    }
    else
		LOG4CXX_ERROR(logger, "Failed in call to QueryDosDevice, GetLastError: " << GetLastError());
  }
  else
  {
    //On 95/98 open up each port to determine their existence

    //Up to 255 COM ports are supported so we iterate through all of them seeing
    //if we can open them or if we fail to open them, get an access denied or general error error.
    //Both of these cases indicate that there is a COM port at that number. 
    for (UINT i=1; i<256; i++)
    {
      //Form the Raw device name
      CString sPort;
      sPort.Format(_T("\\\\.\\COM%d"), i);

      //Try to open the port
      BOOL bSuccess = FALSE;
      HANDLE hPort = ::CreateFile(sPort, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
      if (hPort == INVALID_HANDLE_VALUE)
      {
        DWORD dwError = GetLastError();

        //Check to see if the error was because some other app had the port open or a general failure
        if (dwError == ERROR_ACCESS_DENIED || dwError == ERROR_GEN_FAILURE)
          bSuccess = TRUE;
      }
      else
      {
        //The port was opened successfully
        bSuccess = TRUE;

        //Don't forget to close the port, since we are going to do nothing with it anyway
        CloseHandle(hPort);
      }

      //Add the port number to the array which will be returned
      if (bSuccess)
        ports.Add(i);
    }
  }
}


// **************************************************************************************************************8
// CSerialPortDummy
// **************************************************************************************************************8
IMPLEMENT_DYNAMIC(CSerialPortDummy, CSerialPort)
CSerialPortDummy::CSerialPortDummy()
{
}

CSerialPortDummy::~CSerialPortDummy()
{
}

void CSerialPortDummy::Open(int nPort, DWORD dwBaud, Parity parity, BYTE DataBits, StopBits stopbits, FlowControl fc, BOOL bOverlapped)
{
    UNREFERENCED_PARAMETER (dwBaud);
    UNREFERENCED_PARAMETER (parity);
    UNREFERENCED_PARAMETER (DataBits);
    UNREFERENCED_PARAMETER (stopbits);
    UNREFERENCED_PARAMETER (fc);
    UNREFERENCED_PARAMETER (bOverlapped);

    LOG4CXX_INFO(logger, "Serial port will not be opened: " << nPort);
}

void CSerialPortDummy::Close()
{
}

void CSerialPortDummy::Attach(HANDLE hComm)
{
    UNREFERENCED_PARAMETER (hComm);
}

HANDLE CSerialPortDummy::Detach()
{
	return INVALID_HANDLE_VALUE;
}

CSerialPortDummy::operator HANDLE() const
{
	return INVALID_HANDLE_VALUE;
}

BOOL CSerialPortDummy::IsOpen() const
{
	return TRUE;
}

DWORD CSerialPortDummy::Read(void* lpBuf, DWORD dwCount)
{
    UNREFERENCED_PARAMETER (lpBuf);
    UNREFERENCED_PARAMETER (dwCount);

    return 0;
}

BOOL CSerialPortDummy::Read(void* lpBuf, DWORD dwCount, OVERLAPPED& overlapped)
{
    UNREFERENCED_PARAMETER (lpBuf);
    UNREFERENCED_PARAMETER (dwCount);
    UNREFERENCED_PARAMETER (overlapped);
	
    return FALSE;
}

void CSerialPortDummy::ReadEx(void* lpBuf, DWORD dwCount)
{
    UNREFERENCED_PARAMETER (lpBuf);
    UNREFERENCED_PARAMETER (dwCount);
}

DWORD CSerialPortDummy::Write(const void* lpBuf, DWORD dwCount)
{
    UNREFERENCED_PARAMETER (lpBuf);
    UNREFERENCED_PARAMETER (dwCount);

    return 0;
}

BOOL CSerialPortDummy::Write(const void* lpBuf, DWORD dwCount, OVERLAPPED& overlapped)
{
    UNREFERENCED_PARAMETER (lpBuf);
    UNREFERENCED_PARAMETER (dwCount);
    UNREFERENCED_PARAMETER (overlapped);

    return FALSE;
}

void CSerialPortDummy::WriteEx(const void* lpBuf, DWORD dwCount)
{
    UNREFERENCED_PARAMETER (lpBuf);
    UNREFERENCED_PARAMETER (dwCount);
}

void CSerialPortDummy::TransmitChar(char cChar)
{
    UNREFERENCED_PARAMETER (cChar);
}

void CSerialPortDummy::GetOverlappedResult(OVERLAPPED& overlapped, DWORD& dwBytesTransferred, BOOL bWait)
{
    UNREFERENCED_PARAMETER (overlapped);
    UNREFERENCED_PARAMETER (dwBytesTransferred);
    UNREFERENCED_PARAMETER (bWait);
}

void CSerialPortDummy::CancelIo()
{
}

void CSerialPortDummy::GetConfig(COMMCONFIG& config)
{
    UNREFERENCED_PARAMETER (config);
}

void CSerialPortDummy::SetConfig(COMMCONFIG& Config)
{
    UNREFERENCED_PARAMETER (Config);
}

void CSerialPortDummy::ClearBreak()
{
}

void CSerialPortDummy::SetBreak()
{
}

void CSerialPortDummy::ClearError(DWORD& dwErrors)
{
    UNREFERENCED_PARAMETER (dwErrors);
}

void CSerialPortDummy::GetStatus(COMSTAT& stat)
{
    UNREFERENCED_PARAMETER (stat);
}

void CSerialPortDummy::GetState(DCB& dcb)
{
    UNREFERENCED_PARAMETER (dcb);
}

void CSerialPortDummy::SetState(DCB& dcb)
{
    UNREFERENCED_PARAMETER (dcb);
}

void CSerialPortDummy::Escape(DWORD dwFunc)
{
    UNREFERENCED_PARAMETER (dwFunc);
}

void CSerialPortDummy::ClearDTR()
{
}

void CSerialPortDummy::ClearRTS()
{
}

void CSerialPortDummy::SetDTR()
{
}

void CSerialPortDummy::SetRTS()
{
}

void CSerialPortDummy::SetXOFF()
{
}

void CSerialPortDummy::SetXON()
{
}

void CSerialPortDummy::GetProperties(COMMPROP& properties)
{
    UNREFERENCED_PARAMETER (properties);
}

void CSerialPortDummy::GetModemStatus(DWORD& dwModemStatus)
{
    UNREFERENCED_PARAMETER (dwModemStatus);
}
 
void CSerialPortDummy::SetTimeouts(COMMTIMEOUTS& timeouts)
{
    UNREFERENCED_PARAMETER (timeouts);
}

void CSerialPortDummy::GetTimeouts(COMMTIMEOUTS& timeouts)
{
    UNREFERENCED_PARAMETER (timeouts);
}

void CSerialPortDummy::Set0Timeout()
{
}

void CSerialPortDummy::Set0WriteTimeout()
{
}

void CSerialPortDummy::Set0ReadTimeout()
{
}

void CSerialPortDummy::SetMask(DWORD dwMask)
{
    UNREFERENCED_PARAMETER (dwMask);
}

void CSerialPortDummy::GetMask(DWORD& dwMask)
{
    UNREFERENCED_PARAMETER (dwMask);
}

void CSerialPortDummy::WaitEvent(DWORD& dwMask)
{
    UNREFERENCED_PARAMETER (dwMask);
}

void CSerialPortDummy::WaitEvent(DWORD& dwMask, OVERLAPPED& overlapped)
{
    UNREFERENCED_PARAMETER (dwMask);
    UNREFERENCED_PARAMETER (overlapped);
}

void CSerialPortDummy::Flush()
{
}

void CSerialPortDummy::Purge(DWORD dwFlags)
{
    UNREFERENCED_PARAMETER (dwFlags);
}

void CSerialPortDummy::TerminateOutstandingWrites()
{
}

void CSerialPortDummy::TerminateOutstandingReads()
{
}

void CSerialPortDummy::ClearWriteBuffer()
{
}

void CSerialPortDummy::ClearReadBuffer()
{
}

void CSerialPortDummy::Setup(DWORD dwInQueue, DWORD dwOutQueue)
{
    UNREFERENCED_PARAMETER (dwInQueue);
    UNREFERENCED_PARAMETER (dwOutQueue);
}

void CSerialPortDummy::OnCompletion(DWORD dwErrorCode, DWORD dwCount, LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER (dwErrorCode);
    UNREFERENCED_PARAMETER (dwCount);
    UNREFERENCED_PARAMETER (lpOverlapped);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// A class for communicating with the AR2-Relay interface for communication the Pass/Fail status of a run.
//////////////////////////////////////////////////////////////////////////////////////////////////////////
CSerialPortPassFail::CSerialPortPassFail()
:CSerialPort()
{
}

CSerialPortPassFail::~CSerialPortPassFail()
{
}

void CSerialPortPassFail::SetPass()
{
	__super::SetRTS();
}

void CSerialPortPassFail::SetFail()
{
	__super::SetDTR();
}

void CSerialPortPassFail::ClearPass()
{
	__super::ClearRTS();	
}

void CSerialPortPassFail::ClearFail()
{
	__super::ClearDTR();
}

void CSerialPortPassFail::ClearPassFail()
{
	ClearPass();
	ClearFail();
}