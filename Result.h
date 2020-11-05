
// Name: WebServer.h
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#include <Arduino.h>

struct Result
{
	Result(bool success, const String& message) 
	  : m_message(message)
	  , m_success(success) {}
	operator bool() const { return m_success; }
	const String& message() const { return m_message; }
	
private:
	const String m_message;
	const bool m_success;
};
