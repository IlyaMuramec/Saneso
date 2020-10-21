#include "stdafx.h"
#include "MessageQueue.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

log4cxx::LoggerPtr CMessageQueue::logger = log4cxx::Logger::getLogger("MessageQueue");

critical_section::critical_section(void)
{
	::InitializeCriticalSection(this); 
}

critical_section::~critical_section(void) 
{ 
	::DeleteCriticalSection(this); 
}

void critical_section::lock() 
{ 
	::EnterCriticalSection(this); 
}

void critical_section::unlock() 
{ 
	::LeaveCriticalSection(this); 
}

scoped_lock::scoped_lock(critical_section & cs) : 
	m_cs(cs) 
{ 
	m_cs.lock();
}

scoped_lock::~scoped_lock(void) 
{ 
	m_cs.unlock(); 
}

condition_variable::condition_variable() 
{ 
	::InitializeConditionVariable(this); 
}

void condition_variable::wait(scoped_lock & lock, DWORD dwMilliseconds)
{
	if (!SleepConditionVariableCS(this, &lock.m_cs, dwMilliseconds))
	{
		DWORD error = ::GetLastError();
		std::string msg = "SleepConditionVariableCS() failed with error code = " + error;
		throw std::runtime_error(msg);
	}
}
void condition_variable::wait(scoped_lock & lock, std::function<bool()> predicate, DWORD dwMilliseconds)
{
	while (!predicate())  
	{ 
		wait(lock, dwMilliseconds);
	}
}

void condition_variable::notify_one() 
{ 
	::WakeConditionVariable(this); 
}

void condition_variable::notify_all() 
{ 
	::WakeAllConditionVariable(this);
}

CMessageQueue::CMessageQueue() :
	m_critical_section()
{
}

void CMessageQueue::Push(CMessage* const & input)
{
	scoped_lock lock(m_critical_section); //lock
	m_internal_queue.push(input);
	m_queue_empty.notify_all();
}

CMessage* CMessageQueue::Pop()
{
	scoped_lock lock(m_critical_section); //lock
	m_queue_empty.wait(lock, [&]{ return !Empty(); }, 500);
	if (m_internal_queue.empty())
	{
		return NULL;
	}
	CMessage* output = m_internal_queue.front();
	m_internal_queue.pop();
	return output;
}

bool CMessageQueue::Empty() const
{
	scoped_lock lock(m_critical_section);
	return m_internal_queue.empty();
}

int CMessageQueue::Size() const
{
  scoped_lock lock(m_critical_section);
  return m_internal_queue.size();
}

