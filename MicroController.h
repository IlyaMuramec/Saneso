#pragma once

#include "afxmt.h"
#include "MessageQueue.h"
#include <string>
#include <vector>
#include <queue>
#include <log4cxx\logger.h>
#include "FramesPerSecond.h"
#include "CameraSettings.h"

class CScopeStatus;
class CSerialPort;

typedef void(*ConnectCallbackType)(void *);
typedef void(*MessageCallbackType)(void *, CMessage *pMsg);

enum EEPromLocation
{
	ProcessorBox,
	Scope
};

class CMicroController
{
public:
	CMicroController(CScopeStatus *pScopeStatus);
	~CMicroController();

	int ToggleUSBDeviceState();
	int Connect();
	std::string GetVersion()	{ return m_sVersion; }
	std::string GetStatus()		{ return m_sStatus; }
	void SetCameraChipType(int board, CameraChip chip)	{ m_CameraChipMap[board] = chip; }
	int GetShutterValue(int board);
	int GetGainValue(int board);
	int SetTestPattern(int board, bool enable);
	int SetAutoShutterGain(int board, bool autoShutter, bool autoGain);
	int SetShutterValue(int board, int value);
	int SetGainValue(int board, int value);
	int SetWhiteBalanceEnable(int board, bool enable);
	int SetWhiteBalanceGains(int board, float red, float green, float blue);
	int ReadCameraRegister(int board, unsigned int reg, unsigned short &value);
	int WriteCameraRegister(int board, unsigned int reg, unsigned short value, bool onetime = false);
	int WriteCameraHighLowRegister(int board, unsigned highReg, unsigned lowReg, unsigned int value);
	int SendCameraInit(int board, bool showError);
	int SetLEDValue(int channel, float value);
	int SetLEDValues(std::map<int, float> values);
	int SetLEDLevel(bool front, int level);
	int SetShadowCast(int delayMs, int bitmask1, int bitmask2);
	int SetLEDMUXPeriod(unsigned int period);
	void ToggleCameraResetLine(bool bRecovery);
	void ReadAllShutterTimes();
	double GetHeartBeat()	{ return _heartbeatsPerSecond.CalcFPS(); }
	void SendReset();
	void SendReflash();
	int GetConnectedComPort()	{ return _nConnectedComPort; }
	void SetShutterMonitor(int board) { _nShutterMonitorBoard = board;  }
	void SetGainMonitor(int board) { _nGainMonitorBoard = board; }
	std::string ReadEEPromSN(EEPromLocation location);
	void WriteEEPromSN(EEPromLocation location, std::string value);
	std::string ReadEEProm(EEPromLocation location);
	int WriteEEProm(EEPromLocation location, std::string value);
	int ReadEEPromMemoryLocation(EEPromLocation location, unsigned int offset, std::vector<byte> &bytes);
	int WriteEEPromMemoryLocation(EEPromLocation location, unsigned int offset, std::vector<byte> &bytes);
	void WriteEEPromScopeHotSwap();

	void AddConnectCallback(ConnectCallbackType callback, void *pObject) { m_ConnectCallbacks[callback] = pObject; }
	void AddMessageCallback(MessageCallbackType callback, void *pObject) { m_MessageCallbacks[callback] = pObject; }
	void NotifyMessageListeners(CMessage *pMsg);
	void SimulateControlMessages();
private:
	static log4cxx::LoggerPtr logger;

	int TryComPort(int comPort);
	int ReadVersion();
	int ReadStatus();
	int ReadShutterValue(int board);
	int ReadGainValue(int board);
	DWORD SendMesage(const void* lpSendBuf, DWORD dwSendCount, void* lpRecvBuf, DWORD dwRecvCount, bool bLog);
	void OnConnected();
	void Disconnect();
	void EndThreads();
	void SetCameraResetLine(byte bits);

	std::vector<byte> PreprocessForSend(const void* lpSendBuf, DWORD dwSendCount);
	std::vector<byte> PreprocessFromReceive(const void* lpRecvBuf, DWORD dwRecvCount);

	static UINT ReceiveThreadProc(LPVOID param)
	{
		return ((CMicroController*)param)->ReceiveLoop();
	}
	static UINT SimulateThreadProc(LPVOID param)
	{
		return ((CMicroController*)param)->SimulateLoop();
	}
	static UINT MonitorThreadProc(LPVOID param)
	{
		return ((CMicroController*)param)->MonitorLoop();
	}

	UINT ReceiveLoop();
	UINT SimulateLoop();
	UINT MonitorLoop();

	CScopeStatus *m_pScopeStatus;
	CSerialPort *m_pSerialPort;
	CCriticalSection m_CriticalSection;
	CCriticalSection m_CriticalSectionResetCameras;
	std::string m_sVersion;
	std::string m_sStatus;
	bool _bExiting;
	bool _bConnected;
	int _nConnectedComPort;
	int _nShutterMonitorBoard;
	int _nGainMonitorBoard;
	HANDLE _hReceiveThread;
	HANDLE _hSimulateThread;
	HANDLE _hMonitorThread;
	std::vector<int> m_vShutterTimes;
	std::vector<int> m_vGains;
	std::map<int, CameraChip> m_CameraChipMap;
	CMessageQueue m_ReceiveQueue;
	CFramesPerSecond _heartbeatsPerSecond;
	std::map<ConnectCallbackType, void *> m_ConnectCallbacks;
	std::map<MessageCallbackType, void *> m_MessageCallbacks;
};

