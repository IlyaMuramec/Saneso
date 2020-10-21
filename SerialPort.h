/*
Module : SERIALPORT.H
Purpose: Declaration for an MFC wrapper class for serial ports
Created: PJN / 31-05-1999
History: None

Copyright (c) 1999 by PJ Naughter.  
All rights reserved.

*/



///////////////////// Macros / Structs etc //////////////////////////

#ifndef __SERIALPORT_H__
#define __SERIALPORT_H__

#include <log4cxx\logger.h>

/////////////////////////// Classes ///////////////////////////////////////////


////// Serial port exception class ////////////////////////////////////////////

void AfxThrowSerialException(DWORD dwError = 0);

class CSerialException : public CException
{
public:
//Constructors / Destructors
	CSerialException(DWORD dwError);
	~CSerialException();

//Methods
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual BOOL GetErrorMessage(LPTSTR lpstrError, UINT nMaxError,	PUINT pnHelpContext = NULL);
	CString GetErrorMessage();

//Data members
	DWORD m_dwError;

protected:
	DECLARE_DYNAMIC(CSerialException)
};



//// The actual serial port class /////////////////////////////////////////////

class CSerialPort : public CObject
{
public:
//Enums
  enum FlowControl
  {
    NoFlowControl,
    CtsRtsFlowControl,
    CtsDtrFlowControl,
    DsrRtsFlowControl,
    DsrDtrFlowControl,
    XonXoffFlowControl
  };

  enum Parity
  {    
    EvenParity,
    MarkParity,
    NoParity,
    OddParity,
    SpaceParity
  };

  enum StopBits
  {
    OneStopBit,
    OnePointFiveStopBits,
    TwoStopBits
  };

//Constructors / Destructors
  CSerialPort();
  virtual ~CSerialPort();

//General Methods
  virtual void Open(int nPort, DWORD dwBaud = 9600, Parity parity = NoParity, BYTE DataBits = 8, 
            StopBits stopbits = OneStopBit, FlowControl fc = NoFlowControl, BOOL bOverlapped = FALSE);
  virtual void Close();
  virtual void Attach(HANDLE hComm);
  virtual HANDLE Detach();
  virtual operator HANDLE() const { return m_hComm; };
  virtual BOOL IsOpen() const { return m_hComm != INVALID_HANDLE_VALUE; };
#ifdef _DEBUG
  void CSerialPort::Dump(CDumpContext& dc) const;
#endif

//Reading / Writing Methods
  virtual DWORD Read(void* lpBuf, DWORD dwCount);
  virtual BOOL Read(void* lpBuf, DWORD dwCount, OVERLAPPED& overlapped);
  virtual void ReadEx(void* lpBuf, DWORD dwCount);
  virtual DWORD Write(const void* lpBuf, DWORD dwCount);
  virtual BOOL Write(const void* lpBuf, DWORD dwCount, OVERLAPPED& overlapped);
  virtual void WriteEx(const void* lpBuf, DWORD dwCount);
  virtual void TransmitChar(char cChar);
  virtual void GetOverlappedResult(OVERLAPPED& overlapped, DWORD& dwBytesTransferred, BOOL bWait);
  virtual void CancelIo();

//Configuration Methods
  virtual void GetConfig(COMMCONFIG& config);
  static void GetDefaultConfig(int nPort, COMMCONFIG& config);
  virtual void SetConfig(COMMCONFIG& Config);
  static void SetDefaultConfig(int nPort, COMMCONFIG& config);

//Misc RS232 Methods
  virtual void ClearBreak();
  virtual void SetBreak();
  virtual void ClearError(DWORD& dwErrors);
  virtual void GetStatus(COMSTAT& stat);
  virtual void GetState(DCB& dcb);
  virtual void SetState(DCB& dcb);
  virtual void Escape(DWORD dwFunc);
  virtual void ClearDTR();
  virtual void ClearRTS();
  virtual void SetDTR();
  virtual void SetRTS();
  virtual void SetXOFF();
  virtual void SetXON();
  virtual void GetProperties(COMMPROP& properties);
  virtual void GetModemStatus(DWORD& dwModemStatus); 
  virtual int  GetComPortNumber(void)	const {return m_nComPort;};

//Timeouts
  virtual void SetTimeouts(COMMTIMEOUTS& timeouts);
  virtual void GetTimeouts(COMMTIMEOUTS& timeouts);
  virtual void Set0Timeout();
  virtual void Set0WriteTimeout();
  virtual void Set0ReadTimeout();

//Event Methods
  virtual void SetMask(DWORD dwMask);
  virtual void GetMask(DWORD& dwMask);
  virtual void WaitEvent(DWORD& dwMask);
  virtual void WaitEvent(DWORD& dwMask, OVERLAPPED& overlapped);
  
//Queue Methods
  virtual void Flush();
  virtual void Purge(DWORD dwFlags);
  virtual void TerminateOutstandingWrites();
  virtual void TerminateOutstandingReads();
  virtual void ClearWriteBuffer();
  virtual void ClearReadBuffer();
  virtual void Setup(DWORD dwInQueue, DWORD dwOutQueue);

//Overridables
  virtual void OnCompletion(DWORD dwErrorCode, DWORD dwCount, LPOVERLAPPED lpOverlapped);

//Static Methods
  static  void EnumerateSerialPorts(CUIntArray& ports);
  
  static log4cxx::LoggerPtr logger;
  
protected:
  HANDLE m_hComm;       //Handle to the comms port
  BOOL   m_bOverlapped; //Is the port open in overlapped IO
  int    m_nComPort;	//For checking which port this was opened on
  

  static void WINAPI _OnCompletion(DWORD dwErrorCode, DWORD dwCount, LPOVERLAPPED lpOverlapped); 

	DECLARE_DYNAMIC(CSerialPort)
};

class CSerialPortDummy: public CSerialPort
{
public:
//Constructors / Destructors
  CSerialPortDummy();
  virtual ~CSerialPortDummy();

//General Methods
  virtual void Open(int nPort, DWORD dwBaud = 9600, Parity parity = NoParity, BYTE DataBits = 8, StopBits stopbits = OneStopBit, FlowControl fc = NoFlowControl, BOOL bOverlapped = FALSE);
  virtual void Close();
  virtual void Attach(HANDLE hComm);
  virtual HANDLE Detach();
  virtual operator HANDLE() const;
  virtual BOOL IsOpen() const;

//Reading / Writing Methods
  virtual DWORD Read(void* lpBuf, DWORD dwCount);
  virtual BOOL Read(void* lpBuf, DWORD dwCount, OVERLAPPED& overlapped);
  virtual void ReadEx(void* lpBuf, DWORD dwCount);
  virtual DWORD Write(const void* lpBuf, DWORD dwCount);
  virtual BOOL Write(const void* lpBuf, DWORD dwCount, OVERLAPPED& overlapped);
  virtual void WriteEx(const void* lpBuf, DWORD dwCount);
  virtual void TransmitChar(char cChar);
  virtual void GetOverlappedResult(OVERLAPPED& overlapped, DWORD& dwBytesTransferred, BOOL bWait);
  virtual void CancelIo();

//Configuration Methods
  virtual void GetConfig(COMMCONFIG& config);
  static void GetDefaultConfig(int nPort, COMMCONFIG& config);
  virtual void SetConfig(COMMCONFIG& Config);
  static void SetDefaultConfig(int nPort, COMMCONFIG& config);

//Misc RS232 Methods
  virtual void ClearBreak();
  virtual void SetBreak();
  virtual void ClearError(DWORD& dwErrors);
  virtual void GetStatus(COMSTAT& stat);
  virtual void GetState(DCB& dcb);
  virtual void SetState(DCB& dcb);
  virtual void Escape(DWORD dwFunc);
  virtual void ClearDTR();
  virtual void ClearRTS();
  virtual void SetDTR();
  virtual void SetRTS();
  virtual void SetXOFF();
  virtual void SetXON();
  virtual void GetProperties(COMMPROP& properties);
  virtual void GetModemStatus(DWORD& dwModemStatus); 

//Timeouts
  virtual void SetTimeouts(COMMTIMEOUTS& timeouts);
  virtual void GetTimeouts(COMMTIMEOUTS& timeouts);
  virtual void Set0Timeout();
  virtual void Set0WriteTimeout();
  virtual void Set0ReadTimeout();

//Event Methods
  virtual void SetMask(DWORD dwMask);
  virtual void GetMask(DWORD& dwMask);
  virtual void WaitEvent(DWORD& dwMask);
  virtual void WaitEvent(DWORD& dwMask, OVERLAPPED& overlapped);
  
//Queue Methods
  virtual void Flush();
  virtual void Purge(DWORD dwFlags);
  virtual void TerminateOutstandingWrites();
  virtual void TerminateOutstandingReads();
  virtual void ClearWriteBuffer();
  virtual void ClearReadBuffer();
  virtual void Setup(DWORD dwInQueue, DWORD dwOutQueue);

//Overridables
  virtual void OnCompletion(DWORD dwErrorCode, DWORD dwCount, LPOVERLAPPED lpOverlapped);

protected:
	DECLARE_DYNAMIC(CSerialPortDummy)
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// A class for communicating with the AR2-Relay interface for communication the Pass/Fail status of a run.
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSerialPortPassFail: public CSerialPort
{
public:
	CSerialPortPassFail();
	virtual ~CSerialPortPassFail();

	void SetPass();
	void SetFail();
	void ClearPass();
	void ClearFail();
	void ClearPassFail();
};

#endif //__SERIALPORT_H__