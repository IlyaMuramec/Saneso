// Serial.cpp

#include "stdafx.h"
#include "Serial.h"

#pragma warning(disable : 4996)

log4cxx::LoggerPtr CSerial::logger = log4cxx::Logger::getLogger("SerialPort");


CSerial::CSerial()
{

	memset( &m_OverlappedRead, 0, sizeof( OVERLAPPED ) );
 	memset( &m_OverlappedWrite, 0, sizeof( OVERLAPPED ) );
	m_hIDComDev = NULL;
	m_bOpened = FALSE;

}

CSerial::~CSerial()
{

	Close();

}

BOOL CSerial::Open( int nPort, int nBaud )
{

	if( m_bOpened ) return( TRUE );

	CString szPort;
	CString szComParams;
	DCB dcb;

	szPort.Format(_T("COM%d"), nPort);
	m_hIDComDev = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL );
	if( m_hIDComDev == NULL ) return( FALSE );

	memset( &m_OverlappedRead, 0, sizeof( OVERLAPPED ) );
 	memset( &m_OverlappedWrite, 0, sizeof( OVERLAPPED ) );

	COMMTIMEOUTS CommTimeOuts;
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 5000;
	SetCommTimeouts( m_hIDComDev, &CommTimeOuts );

	szComParams.Format(_T("COM%d:%d,n,8,1"), nPort, nBaud );

	m_OverlappedRead.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	m_OverlappedWrite.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	dcb.DCBlength = sizeof( DCB );
	GetCommState( m_hIDComDev, &dcb );
	dcb.BaudRate = nBaud;
	dcb.ByteSize = 8;
	unsigned char ucSet;
	ucSet = (unsigned char) ( ( FC_RTSCTS & FC_DTRDSR ) != 0 );
	ucSet = (unsigned char) ( ( FC_RTSCTS & FC_RTSCTS ) != 0 );
	ucSet = (unsigned char) ( ( FC_RTSCTS & FC_XONXOFF ) != 0 );
	if( !SetCommState( m_hIDComDev, &dcb ) ||
		!SetupComm( m_hIDComDev, 10000, 10000 ) ||
		m_OverlappedRead.hEvent == NULL ||
		m_OverlappedWrite.hEvent == NULL ){
		DWORD dwError = GetLastError();
		if( m_OverlappedRead.hEvent != NULL ) CloseHandle( m_OverlappedRead.hEvent );
		if( m_OverlappedWrite.hEvent != NULL ) CloseHandle( m_OverlappedWrite.hEvent );
		CloseHandle( m_hIDComDev );
		return( FALSE );
		}

	m_bOpened = TRUE;

	return( m_bOpened );

}

BOOL CSerial::Close( void )
{

	if( !m_bOpened || m_hIDComDev == NULL ) return( TRUE );

	if( m_OverlappedRead.hEvent != NULL ) CloseHandle( m_OverlappedRead.hEvent );
	if( m_OverlappedWrite.hEvent != NULL ) CloseHandle( m_OverlappedWrite.hEvent );
	CloseHandle( m_hIDComDev );
	m_bOpened = FALSE;
	m_hIDComDev = NULL;

	return( TRUE );

}

BOOL CSerial::WriteCommByte( unsigned char ucByte )
{
	BOOL bWriteStat;
	DWORD dwBytesWritten;

	bWriteStat = WriteFile( m_hIDComDev, (LPSTR) &ucByte, 1, &dwBytesWritten, &m_OverlappedWrite );
	if( !bWriteStat && ( GetLastError() == ERROR_IO_PENDING ) ){
		if( WaitForSingleObject( m_OverlappedWrite.hEvent, 1000 ) ) dwBytesWritten = 0;
		else{
			GetOverlappedResult( m_hIDComDev, &m_OverlappedWrite, &dwBytesWritten, FALSE );
			m_OverlappedWrite.Offset += dwBytesWritten;
			}
		}

	return( TRUE );

}

int CSerial::SendData( const char *buffer, int size )
{

	if( !m_bOpened || m_hIDComDev == NULL ) return( 0 );

	DWORD dwBytesWritten = 0;
	int i;
	for( i=0; i<size; i++ ){
		WriteCommByte( buffer[i] );
		dwBytesWritten++;
		}

	return( (int) dwBytesWritten );

}

int CSerial::ReadDataWaiting( void )
{

	if( !m_bOpened || m_hIDComDev == NULL ) return( 0 );

	DWORD dwErrorFlags;
	COMSTAT ComStat;

	ClearCommError( m_hIDComDev, &dwErrorFlags, &ComStat );

	return( (int) ComStat.cbInQue );

}

int CSerial::ReadData( void *buffer, int limit )
{

	if( !m_bOpened || m_hIDComDev == NULL ) return( 0 );

	BOOL bReadStatus;
	DWORD dwBytesRead, dwErrorFlags;
	COMSTAT ComStat;

	ClearCommError( m_hIDComDev, &dwErrorFlags, &ComStat );
	if( !ComStat.cbInQue ) return( 0 );

	dwBytesRead = (DWORD) ComStat.cbInQue;
	if( limit < (int) dwBytesRead ) dwBytesRead = (DWORD) limit;

	bReadStatus = ReadFile( m_hIDComDev, buffer, dwBytesRead, &dwBytesRead, &m_OverlappedRead );
	if( !bReadStatus ){
		if( GetLastError() == ERROR_IO_PENDING ){
			WaitForSingleObject( m_OverlappedRead.hEvent, 2000 );
			return( (int) dwBytesRead );
			}
		return( 0 );
		}

	return( (int) dwBytesRead );

}

void CSerial::EnumerateSerialPorts(CUIntArray &ports)
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
			int i = 0;

			for (;;)
			{
				//Get the current device name
				TCHAR* pszCurrentDevice = &szDevices[i];

				//If it looks like "COMX" then
				//add it to the array which will be returned
				int nLen = (int)_tcslen(pszCurrentDevice);
				if (nLen > 3 && _tcsnicmp(pszCurrentDevice, _T("COM"), 3) == 0)
				{
					//Work out the port number
					int nPort = _ttoi(&pszCurrentDevice[3]);
					ports.Add(nPort);
				}

				// Go to next NULL character
				while (szDevices[i] != _T('\0'))
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
		for (UINT i = 1; i<256; i++)
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

