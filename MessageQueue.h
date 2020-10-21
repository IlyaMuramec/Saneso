#pragma once

#include "afxmt.h"
#include <queue>
#include <functional>
#include <windows.h>
#include <log4cxx\logger.h>

// GUI messages to MicroController
#define GET_VERSION_MSG			0x01
#define SPI_BRIDGE_MSG			0x02
#define LED_LEVEL_MSG			0x03
#define SHADOW_CAST_MSG			0x04
#define READ_SERIAL_MSG			0x05
#define WRITE_SERIAL_MSG		0x06
#define LIGHTS_ON_OFF_MSG		0x07
#define GET_STATUS_MSG			0x08

// MicroController resonses
#define GET_VERSION_RESPONSE	0x11
#define SPI_BRIDGE_RESPONSE		0x12
#define LED_LEVEL_RESPONSE		0x13
#define SHADOW_CAST_RESPONSE	0x14
#define READ_SERIAL_RESPONSE	0x15
#define WRITE_SERIAL_RESPONSE	0x16
#define LIGHTS_ON_OFF_RESPONSE	0x17
#define GET_STATUS_RESPONSE		0x18

// ASYNC MicroController messages
#define MC_CONTROL_CHANGED	0x21
#define MC_STATUS_MESSAGE	0x22
#define MC_ERROR_MESSAGE	0x23

// Control definitions
#define CTRL_REMOTE_1			0x1		// 0x1 / 0x0 (On / Off)
#define CTRL_REMOTE_2			0x2		// 0x1 / 0x0 (On / Off)
#define CTRL_REMOTE_3			0x3		// 0x1 / 0x0 (On / Off)
#define CTRL_REMOTE_4			0x4		// 0x1 / 0x0 (On / Off)
#define CTRL_REMOTE_5			0x5		// 0x1 / 0x0 (On / Off)
#define CTRL_FRONT_LIGHT_UP		0x10	// 0x0 - 0xA (New Value)
#define CTRL_FRONT_LIGHT_DOWN	0x11	// 0x0 - 0xA (New Value)
#define CTRL_FRONT_LIGHT_ON_OFF	0x12	// 0x1 / 0x0 (On / Off)
#define CTRL_FRONT_LIGHT_AUTO	0x13	// 0x1 / 0x0 (On / Off)
#define CTRL_SIDE_LIGHT_UP		0x20	// 0x0 - 0xA (New Value)
#define CTRL_SIDE_LIGHT_DOWN	0x21	// 0x0 - 0xA (New Value)
#define CTRL_SIDE_LIGHT_ON_OFF	0x22	// 0x1 / 0x0 (On / Off)
#define CTRL_SIDE_LIGHT_AUTO	0x23	// 0x1 / 0x0 (On / Off)

class critical_section : public CRITICAL_SECTION
{
public:
	critical_section(void);
	~critical_section(void);
	void lock();
	void unlock();
};

class scoped_lock
{
	friend class condition_variable;
	critical_section & m_cs;
public:
	scoped_lock(critical_section & cs);
	~scoped_lock(void);
};

class condition_variable : private CONDITION_VARIABLE
{
public:
	condition_variable();
	void wait(scoped_lock & lock, DWORD dwMilliseconds);
	void wait(scoped_lock & lock, std::function<bool()> predicate, DWORD dwMilliseconds);
	void notify_one();
	void notify_all();
};

class CMessage
{
public:
	int type;
	int size;
	std::vector<byte> msgData;
	std::vector<byte> rawData;
};

class CMessageQueue
{
	static log4cxx::LoggerPtr logger;
	std::queue<CMessage*> m_internal_queue;
	mutable critical_section m_critical_section;
	condition_variable m_queue_empty;
	std::string m_name;

public:
	CMessageQueue();
	void Push(CMessage* const & input);
	CMessage* Pop();
	bool Empty() const;
	int Size() const;
};

