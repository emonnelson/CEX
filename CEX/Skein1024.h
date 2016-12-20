// The GPL version 3 License (GPLv3)
// 
// Copyright (c) 2016 vtdev.com
// This file is part of the CEX Cryptographic library.
// 
// This program is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.If not, see <http://www.gnu.org/licenses/>.
//
// 
// Principal Algorithms:
// The Skein Hash Function Family: <a href="https://www.schneier.com/skein1.3.pdf">Skein V1.1</a>.
// Implementation Details:
// An implementation of the Skein digest with a 1024 bit return size. 
// Written by John Underhill, January 13, 2015
// Contact: develop@vtdev.com

#ifndef _CEX_SKEIN1024_H
#define _CEX_SKEIN1024_H

#include "IDigest.h"
#include "SkeinUbiTweak.h"
#include "Threefish1024.h"

NAMESPACE_DIGEST

/// <summary>
/// An implementation of the Skein digest with a 1024 bit digest return size.
/// <para>SHA-3 finalist: The Skein digest</para>
/// </summary> 
/// 
/// <example>
/// <description>Example using the ComputeHash method:</description>
/// <code>
/// Skein1024 digest;
/// std:vector&lt;byte&gt; hash(digest.DigestSize(), 0);
/// // compute a hash
/// digest.ComputeHash(Input, hash);
/// </code>
/// </example>
/// 
/// <remarks>
/// <description>Implementation Notes:</description>
/// <list type="bullet">
/// <item><description>Block size is 128 bytes, (1024 bits).</description></item>
/// <item><description>Digest size is 128 bytes, (1024 bits).</description></item>
/// <item><description>The <see cref="ComputeHash(byte[])"/> method wraps the <see cref="BlockUpdate(byte[], int, int)"/> and DoFinal methods, and resets the internal state.</description>/></item>
/// <item><description>The <see cref="DoFinal(byte[], int)"/> method does NOT reset the internal state; call <see cref="Reset()"/> to reinitialize.</description></item>
/// </list>
/// 
/// <description>Guiding Publications:</description>
/// <list type="number">
/// <item><description>Skein <a href="http://www.skein-hash.info/sites/default/files/skein1.1.pdf">The Skein digest</a>.</description></item>
/// <item><description>The Skein Hash Function Family <a href="http://www.skein-hash.info/sites/default/files/skein1.1.pdf">Skein V1.1</a>.</description></item>
/// <item><description>Skein <a href="http://www.skein-hash.info/sites/default/files/skein-proofs.pdf">Provable Security</a> Support for the Skein Hash Family.</description></item>
/// <item><description>NIST <a href="http://nvlpubs.nist.gov/nistpubs/ir/2012/NIST.IR.7896.pdf">SHA3 Third-Round Report</a> of the SHA-3 Cryptographic Hash Algorithm Competition>.</description></item>
/// </list>
/// </remarks>
class Skein1024 : public IDigest
{
private:

	static const size_t BLOCK_SIZE = 128;
	static const size_t DIGEST_SIZE = 128;
	static const size_t STATE_SIZE = 1024;
	static const size_t STATE_BYTES = STATE_SIZE / 8;
	static const size_t STATE_WORDS = STATE_SIZE / 64;
	static const size_t STATE_OUTPUT = (STATE_SIZE + 7) / 8;

	size_t m_bytesFilled;
	Threefish1024 m_blockCipher;
	std::vector<ulong> m_cipherInput;
	std::vector<ulong> m_configString;
	std::vector<ulong> m_configValue;
	std::vector<ulong> m_digestState;
	SkeinStateType m_initializationType;
	std::vector<byte> m_inputBuffer;
	bool m_isDestroyed;
	SkeinUbiTweak m_ubiParameters;

public:

	Skein1024(const Skein1024&) = delete;
	Skein1024& operator=(const Skein1024&) = delete;
	Skein1024& operator=(Skein1024&&) = delete;

	//~~~Properties~~~//

	/// <summary>
	/// Get: The Digests internal blocksize in bytes
	/// </summary>
	virtual size_t BlockSize() { return BLOCK_SIZE; }

	/// <summary>
	/// Get: Size of returned digest in bytes
	/// </summary>
	virtual size_t DigestSize() { return DIGEST_SIZE; }

	/// <summary>
	/// Get: The digests type name
	/// </summary>
	virtual Digests Enumeral() { return Digests::Skein1024; }

	/// <summary>
	/// Get: The digests class name
	/// </summary>
	virtual const std::string Name() { return "Skein1024"; }

	/// <summary>
	/// Get the post-chain configuration value
	/// </summary>
	std::vector<ulong> GetConfigValue()
	{
		return m_configValue;
	}

	/// <summary>
	/// Get the pre-chain configuration string
	/// </summary>
	std::vector<ulong> GetConfigString()
	{
		return m_configString;
	}

	/// <summary>
	/// Get the initialization type
	/// </summary>
	SkeinStateType GetInitializationType()
	{
		return m_initializationType;
	}

	/// <summary>
	/// Get the state size in bits
	/// </summary>
	size_t GetStateSize()
	{
		return STATE_SIZE;
	}

	/// <summary>
	/// Ubi Tweak parameters
	/// </summary>
	SkeinUbiTweak GetUbiParameters()
	{
		return m_ubiParameters;
	}

	//~~~Constructor~~~//

	/// <summary>
	/// Initialize the digest
	/// </summary>
	explicit Skein1024(SkeinStateType InitializationType = SkeinStateType::Normal)
		:
		m_bytesFilled(0),
		m_blockCipher(),
		m_cipherInput(STATE_WORDS),
		m_configString(STATE_SIZE),
		m_configValue(STATE_SIZE),
		m_digestState(STATE_WORDS),
		m_initializationType(InitializationType),
		m_inputBuffer(STATE_BYTES),
		m_isDestroyed(false),
		m_ubiParameters()
	{
		// generate the configuration string
		m_configString[1] = (ulong)(DigestSize() * 8);
		// "SHA3"
		std::vector<byte> schema(4, 0);
		schema[0] = 83;
		schema[1] = 72;
		schema[2] = 65;
		schema[3] = 51;
		SetSchema(schema);
		SetVersion(1);
		GenerateConfiguration();
		Initialize(InitializationType);
	}

	/// <summary>
	/// Finalize objects
	/// </summary>
	virtual ~Skein1024()
	{
		Destroy();
	}

	//~~~Public Methods~~~//

	/// <summary>
	/// Update the buffer
	/// </summary>
	/// 
	/// <param name="Input">Input data</param>
	/// <param name="InOffset">The starting offset within the Input array</param>
	/// <param name="Length">Amount of data to process in bytes</param>
	///
	/// <exception cref="CryptoDigestException">Thrown if the input buffer is too short</exception>
	virtual void BlockUpdate(const std::vector<byte> &Input, size_t InOffset, size_t Length);

	/// <summary>
	/// Get the Hash value
	/// </summary>
	/// 
	/// <param name="Input">Input data</param>
	/// <param name="Output">The hash output value array</param>
	virtual void ComputeHash(const std::vector<byte> &Input, std::vector<byte> &Output);

	/// <summary>
	/// Release all resources associated with the object
	/// </summary>
	virtual void Destroy();

	/// <summary>
	/// Do final processing and get the hash value
	/// </summary>
	/// 
	/// <param name="Output">The Hash output value array</param>
	/// <param name="OutOffset">The starting offset within the Output array</param>
	/// 
	/// <returns>Size of Hash value</returns>
	///
	/// <exception cref="CryptoDigestException">Thrown if the output buffer is too short</exception>
	virtual size_t DoFinal(std::vector<byte> &Output, const size_t OutOffset);

	/// <summary>
	/// Generate a configuration using a state key
	/// </summary>
	/// 
	/// <param name="InitialState">Twofish Cipher key</param>
	void GenerateConfiguration(std::vector<ulong> InitialState);

	/// <summary>
	/// Used to re-initialize the digest state.
	/// <para>Creates the initial state with zeros instead of the configuration block, then initializes the hash. 
	/// This does not start a new UBI block type, and must be done manually.</para>
	/// </summary>
	/// 
	/// <param name="InitializationType">Initialization parameters</param>
	void Initialize(SkeinStateType InitializationType);

	/// <summary>
	/// Reset the internal state
	/// </summary>
	void Reset();

	/// <summary>
	/// Set the tree height. Tree height must be zero or greater than 1.
	/// </summary>
	/// 
	/// <param name="Height">Tree height</param>
	/// 
	/// <exception cref="Exception::CryptoDigestException">Thrown if an invalid tree height is used</exception>
	void SetMaxTreeHeight(const byte Height);

	/// <summary>
	/// Set the Schema. Schema must be 4 bytes.
	/// </summary>
	/// 
	/// <param name="Schema">Schema Configuration string</param>
	/// 
	/// <exception cref="Exception::CryptoDigestException">Thrown if an invalid schema is used</exception>
	void SetSchema(const std::vector<byte> &Schema);

	/// <summary>
	/// Set the tree fan out size
	/// </summary>
	/// 
	/// <param name="Size">Fan out size</param>
	void SetTreeFanOutSize(const byte Size);

	/// <summary>
	/// Set the tree leaf size
	/// </summary>
	/// 
	/// <param name="Size">Leaf size</param>
	void SetTreeLeafSize(const byte Size);

	/// <summary>
	/// Set the version string. Version must be between 0 and 3, inclusive.
	/// </summary>
	/// 
	/// <param name="Version">Version string</param>
	/// 
	/// <exception cref="Exception::CryptoDigestException">Thrown if an invalid version is used</exception>
	void SetVersion(const uint Version);

	/// <summary>
	/// Update the message digest with a single byte
	/// </summary>
	/// 
	/// <param name="Input">Input byte</param>
	void Update(byte Input);

private:
	void GenerateConfiguration();
	void Initialize();
	void ProcessBlock(uint Value);
	static void PutBytes(std::vector<ulong> Input, std::vector<byte> &Output, size_t Offset, size_t ByteCount);
};

NAMESPACE_DIGESTEND
#endif
