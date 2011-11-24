/*
 * Exceptions.cpp
 *
 *  Created on: 11 Sep 2010
 *      Author: martin
 */

#include "sessionEx.h"

// namespace muroa {

CException::CException(std::string reason) throw () {
	m_reason = reason;
}

CException::~CException() throw () {
}

const char* CException::what() const throw () {
	return m_reason.c_str();
}

const std::string CException::reason() const throw () {
	return m_reason;
}

CApiMisuseException::CApiMisuseException(std::string reason) throw () {
	m_reason = reason;
}

CApiMisuseException::~CApiMisuseException() throw () {
}

const char* CApiMisuseException::what() const throw () {
	return m_reason.c_str();
}

const std::string CApiMisuseException::reason() const throw () {
	return m_reason;
}

InvalidMsgException::InvalidMsgException(std::string reason) throw () {
	m_reason = reason;
}

InvalidMsgException::~InvalidMsgException() throw () {
}

const char* InvalidMsgException::what() const throw () {
	return m_reason.c_str();
}

const std::string InvalidMsgException::reason() const throw () {
	return m_reason;
}

NeedMoreDataException::NeedMoreDataException(std::string reason, int code) throw () {
	m_reason = reason;
	m_code = code;
}

NeedMoreDataException::~NeedMoreDataException() throw () {
}

const char* NeedMoreDataException::what() const throw () {
	return m_reason.c_str();
}

const std::string NeedMoreDataException::reason() const throw () {
	return m_reason;
}

const int NeedMoreDataException::code() const throw () {
	return m_code;
}

//}
