#include "CryptoDigestException.h"

NAMESPACE_EXCEPTION

CryptoDigestException::CryptoDigestException(const std::string &Location, const std::string &Origin, const std::string &Message, ErrorCodes ErrorCode)
	:
	CryptoException(Location, Origin, Message, ErrorCode)
{
}

const ExceptionTypes CryptoDigestException::Enumeral()
{
	return ExceptionTypes::CryptoDigestException;
}

const std::string CryptoDigestException::Name()
{
	return Enumeration::ExceptionTypeConvert::ToName(Enumeral());
}

NAMESPACE_EXCEPTIONEND
