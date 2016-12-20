#ifndef _CEX_STREAMCIPHERS_H
#define _CEX_STREAMCIPHERS_H

#include "CexDomain.h"

NAMESPACE_ENUMERATION
/// <summary>
/// Stream cipher enumeration names
/// </summary>
enum class StreamCiphers : uint8_t
{
	/// <summary>
	/// No stream cipher is specified
	/// </summary>
	None = 0,
	/// <summary>
	/// An implementation of the ChaCha Stream Cipher
	/// </summary>
	ChaCha20 = 16,
	/// <summary>
	/// A Salsa20 Stream Cipher
	/// </summary>
	Salsa = 32
};

NAMESPACE_ENUMERATIONEND
#endif