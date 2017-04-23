﻿#ifndef _CEX_ZEROPAD_H
#define _CEX_ZEROPAD_H

#include "IPadding.h"

NAMESPACE_PADDING

/// <summary>
/// The Zero Padding Scheme (Not Recommended).
/// </summary>
class ZeroPad final : public IPadding
{
private:

	ZeroPad(const ZeroPad&) = delete;
	ZeroPad& operator=(const ZeroPad&) = delete;
	ZeroPad& operator=(ZeroPad&&) = delete;

	static const std::string CLASS_NAME;

public:

	//~~~Properties~~~//

	/// <summary>
	/// Get: The padding modes type name
	/// </summary>
	const PaddingModes Enumeral() override;

	/// <summary>
	/// Get: The padding modes class name
	/// </summary>
	const std::string &Name() override;

	//~~~Constructor~~~//

	/// <summary>
	/// CTor: Instantiate this class
	/// </summary>
	ZeroPad();

	/// <summary>
	/// Destructor
	/// </summary>
	~ZeroPad() override;

	//~~~Public Functions~~~//

	/// <summary>
	/// Add padding to input array
	/// </summary>
	///
	/// <param name="Input">Array to modify</param>
	/// <param name="Offset">Offset into array</param>
	///
	/// <returns>Length of padding</returns>
	///
	/// <exception cref="Exception::CryptoPaddingException">Thrown if the padding offset value is longer than the array length</exception>
	size_t AddPadding(std::vector<byte> &Input, size_t Offset) override;

	/// <summary>
	/// Get the length of padding in an array
	/// </summary>
	///
	/// <param name="Input">Padded array of bytes</param>
	///
	/// <returns>Length of padding</returns>
	size_t GetPaddingLength(const std::vector<byte> &Input) override;

	/// <summary>
	/// Get the length of padding in an array
	/// </summary>
	///
	/// <param name="Input">Padded array of bytes</param>
	/// <param name="Offset">Offset into array</param>
	///
	/// <returns>Length of padding</returns>
	size_t GetPaddingLength(const std::vector<byte> &Input, size_t Offset) override;
};

NAMESPACE_PADDINGEND
#endif

