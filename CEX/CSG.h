﻿// The GPL version 3 License (GPLv3)
// 
// Copyright (c) 2017 vtdev.com
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
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// 
// Implementation Details:
// An implementation of a cSHAKE Generator (CSG)
// Written by John Underhill, February 23, 2018
// Contact: develop@vtdev.com

#ifndef CEX_CSG_H
#define CEX_CSG_H

#include "IDrbg.h"
#include "IProvider.h"
#include "SHAKE.h"
#include "ShakeModes.h"

NAMESPACE_DRBG

using Enumeration::ShakeModes;
using Kdf::SHAKE;
using Provider::IProvider;
using Enumeration::Providers;

/// <summary>
/// An implementation of an cSHAKE Generator DRBG
/// </summary> 
/// 
/// <example>
/// <description>Generate an array of pseudo random bytes:</description>
/// <code>
/// CSG gen(ShakeModes::SHAKE256, [Providers::CSP]);
/// // initialize
/// gen.Initialize(Seed, [Nonce], [Info]);
/// // generate bytes
/// gen.Generate(Output, [Offset], [Size]);
/// </code>
/// </example>
/// 
/// <remarks>
///
/// <para><EM>Initialize</EM> \n
/// The Initialize function can take up to 3 inputs; the generator Seed which is the primary key, a Nonce value which acts as a customization string, \n
/// and the distribution code (Info parameter) used as the Name parameter in SHAKE. \n
/// The initialization parameters determine the type of underlying generator is invoked. If only a key is used, the generator invokes a SHAKE instance. \n
/// if both the Key and Nonce parameter are used to seed the generator, and instance of simple-cSHAKE is invoked, and if all three parameters contain keying material \n
/// (Key, Nonce, and Info), an instance of cSHAKE is invoked.
/// </para>
///
/// <para><EM>Generate</EM> \n
/// The generate function employs a state counter, that will automatically trigger a re-seed of the cSHAKE instance after a user defined maximum threshold has been exceeded. \n
/// Use the ReseedThreshold parameter to tune the auto re-seed interval.
/// 
/// </para>
///
/// <description><B>Overview:</B></description>
/// <para>The HMAC based generator uses a hash function in a keyed HMAC to generate pseudo-random output. \n
/// The HMAC is first initialized with the input seed values, then used in an internal key strengthening/derivation function to extract a key equal to the underlying hash functions internal block size,
/// this is the most secure configuration when using a random HMAC key. \n
/// The key derivation function takes as input a seed counter, (which is incremented by the number of bytes generated in each expansion cycle), the initial seed key, 
/// and uses an entropy provider to pad the input blocks to the HMAC, so that the hash function processes a full block of state in the hash finalizer function. \n
/// In a Merkle–Damgård construction (SHA2), the finalizer appends a code to the end of the last block, (and if the block is full, it processes a block of zero-byte padding with the code), 
/// this is compensated for by subtracting the codes length from the random padding request length when required. \n
/// The generator function uses the re-keyed HMAC to process a state counter, (optionally initialized as a random value array, and incremented on each cycle iteration by the required number of bytes copied from a block), 
/// an initial random state array generated by the random provider, and the optional DistributionCode array, (set either through the property or the Info parameter of the Initialize function). \n
/// The DistributionCode can be applied as a secondary, static source of entropy used by the generate function, making it similar to an HKDF construction, (CSG uses an 8 byte counter instead of 1 byte used by HKDF). 
/// The DistributionCodeMax property is the ideal size for the code, (it also compensates for any hash finalizer code length), this ensures only full blocks are processed by the hash function finalizer. \n
/// The generator copies the HMAC finalized output to the internal state, and the functions output array. 
/// The pseudo-random state array is processed as seed material in the next iteration of the generation cycle, in a continuous transformation process. \n
/// The state counter can be initialized by the Nonce parameter of the Initialize function to an 8 byte secret and random value, (this is strongly recommended). \n
/// The reseed-requests counter is incremented by the number of bytes processed by a generation call, if this value exceeds the ReseedThreshold value (10 * the MAC output size by default),
/// the generator transforms the state to an internal array, (not added to output), and uses that state, along with the reseed counter and entropy provider, to buid a new HMAC key. \n
/// The HMAC is then re-keyed, the reseed-requests counter is reset, and a new initial state is generated by the entropy provider for the next generation cycle.
/// </para>
/// 
/// <description><B>Initialization and Update:</B></description>
/// <para>The Initialize functions have three different parameter options: the Seed which is the primary key, 
/// the Nonce used to initialize the internal state-counter, and the Info which is used in the Generate function. \n
/// The Seed value must be one of the LegalKeySizes() in length, and must be a secret and random value. \n
/// The supported seed-sizes are calculated based on the hash functions internal block size, and can vary depending on which message digest is used to instantiate the generator. \n
/// The eight byte (NonceSize) Nonce value is another secret value, used to initialize the internal state counter to a non-zero random value. \n
/// The Info parameter maps to the DistributionCode() property, and is used as message state when generating the pseudo-random output. \n
/// The DistributionCode is recommended, and for best security, should be secret, random, and equal in length to the DistributionCodeMax() property \n 
/// The Update function uses the seed value to re-key the HMAC via the internal key derivation function. \n
/// The update functions Seed parameter, must be a random seed value equal in length to the seed used to initialize the generator.</para>
///
/// <description><B>Predictive Resistance:</B></description>
/// <para>Predictive and backtracking resistance prevent an attacker who has gained knowledge of generator state at some time from predicting future or previous outputs from the generator. \n
/// The optional resistance mechanism uses an entropy provider to add seed material to the generator, this new seed material is passed through the derivation function along with the current state, 
/// the output hash is used to reseed the generator. \n
/// The default interval at which this reseeding occurs is 1000 times the digest output size in bytes, but can be set using the ReseedThreshold() property; once this number of bytes or greater has been generated, 
/// the seed is regenerated. \n 
/// Predictive resistance is strongly recommended when producing large amounts of pseudo-random (10kb or greater).</para>
///
/// <description>Implementation Notes:</description>
/// <list type="bullet">
/// <item><description>The class constructor can either be initialized with a SHAKE instance type, and entropy provider instances, or using the ShakeModes and Providers enumeration names.</description></item>
/// <item><description>The provider instance created using the enumeration constructor, is automatically deleted when the class is destroyed.</description></item>
/// <item><description>The generator can be initialized with either a SymmetricKey key container class, or with a Seed and optional inputs of Nonce and Info.</description></item>
/// <item><description>The LegalKeySizes() property contains a list of the recommended seed input sizes.</description></item>
/// <item><description>There are three legal seed sizes; the first (smallest) is the minimum required key size, the second the recommended size, and the third is maximum security.</description></item>
/// <item><description>Initializing with a Nonce is recommended; the nonce value must be random, secret, and 8 bytes in length.</description></item>
/// <item><description>The Info value (DistributionCode) is also recommended; for best security, this value should be secret, random, and DistributionCodeMax() in length.</description></item>
/// <item><description>The Generate() methods can not be used until an Initialize() function has been called, and the generator is seeded.</description></item>
/// <item><description>The Update() method requires a Seed of length equal to the seed used to initialize the generator.</description></item>
/// </list>
/// 
/// <description>Guiding Publications:</description>
/// <list type="number">
/// <item><description>Fips-202: The <a href="http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf">SHA-3 Standard</a></description>.</item>
/// <item><description>SP800-185: <a href="http://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-185.pdf">SHA-3 Derived Functions</a></description></item>
/// </list>
/// </remarks>
class CSG final : public IDrbg
{
private:

	enum ShakeType : byte
	{
		Shake = 1,
		scShake = 2,
		cShake = 3
	};

	static const std::string CLASS_NAME;
	// max-out: 35184372088832, max-request: 65536, max-reseed: 536870912
	static const ulong MAX_OUTPUT = 35184372088832;
	static const size_t MAX_REQUEST = 65536;
	static const size_t MAX_RESEED = 536870912;

	size_t m_blockSize;
	std::vector<byte> m_customNonce;
	bool m_destroyEngine;
	std::vector<byte> m_distributionCode;
	size_t m_distributionCodeMax;
	bool m_isDestroyed;
	bool m_isInitialized;
	std::vector<SymmetricKeySize> m_legalKeySizes;
	bool m_prdResistant;
	std::unique_ptr<IProvider> m_providerSource;
	Providers m_providerType;
	size_t m_reseedCounter;
	size_t m_reseedRequests;
	size_t m_reseedThreshold;
	size_t m_secStrength;
	size_t m_seedSize;
	std::unique_ptr<SHAKE> m_shakeEngine;
	ShakeModes m_shakeMode;
	ShakeType m_shakeType;

public:

	//~~~Constructor~~~//

	/// <summary>
	/// Copy constructor: copy is restricted, this function has been deleted
	/// </summary>
	CSG(const CSG&) = delete;

	/// <summary>
	/// Copy operator: copy is restricted, this function has been deleted
	/// </summary>
	CSG& operator=(const CSG&) = delete;

	/// <summary>
	/// Instantiate the class using a block cipher type name, and an optional entropy source type
	/// </summary>
	///
	/// <param name="ShakeMode">The underlying SHAKE implementation mode</param>
	/// <param name="ProviderType">The enumeration type name of an entropy source; enables predictive resistance</param>
	///
	/// <exception cref="Exception::CryptoGeneratorException">Thrown if an unrecognized digest type name is used</exception>
	explicit CSG(ShakeModes ShakeMode = ShakeModes::SHAKE256, Providers ProviderType = Providers::ACP);

	/// <summary>
	/// Instantiate the class using a digest instance, and an optional entropy source 
	/// </summary>
	/// 
	/// <param name="ShakeMode">The underlying shake implementation mode</param>
	/// <param name="Provider">Provides an entropy source; enables predictive resistance, can be null</param>
	/// 
	/// <exception cref="Exception::CryptoGeneratorException">Thrown if a null digest is used</exception>
	explicit CSG(ShakeModes ShakeMode, IProvider* Provider = 0);

	/// <summary>
	/// Destructor: finalize this class
	/// </summary>
	~CSG() override;

	//~~~Accessors~~~//

	/// <summary>
	/// Read/Write: Reads or Sets the personalization string value in the KDF initialization parameters.
	/// <para>Must be set before <see cref="Initialize(ISymmetricKey)"/> is called.
	/// Changing this code will create a unique distribution of the generator.
	/// Code can be sized as either a zero byte array, or any length up to the DistributionCodeMax size.
	/// For best security, the distribution code should be random, secret, and equal in length to the DistributionCodeMax() size.</para>
	/// </summary>
	std::vector<byte> &DistributionCode() override;

	/// <summary>
	/// Read Only: The maximum size of the distribution code in bytes.
	/// <para>The distribution code can be used as a secondary source of entropy (secret) in the KDF key expansion phase.
	/// For best security, the distribution code should be random, secret, and equal in size to this value.</para>
	/// </summary>
	const size_t DistributionCodeMax() override;

	/// <summary>
	/// Read Only: The Drbg generators type name
	/// </summary>
	const Drbgs Enumeral() override;

	/// <summary>
	/// Read Only: Generator is ready to produce random
	/// </summary>
	const bool IsInitialized() override;

	/// <summary>
	/// Read Only: The legal input seed sizes in bytes
	/// </summary>
	std::vector<SymmetricKeySize> LegalKeySizes() const override;

	/// <summary>
	/// Read Only: The maximum number of bytes that can be generated with a generator instance
	/// </summary>
	const ulong MaxOutputSize() override;

	/// <summary>
	/// Read Only: The maximum number of bytes that can be generated in a single request
	/// </summary>
	const size_t MaxRequestSize() override;

	/// <summary>
	/// Read Only: The maximum number of times the generator can be reseeded
	/// </summary>
	const size_t MaxReseedCount() override;

	/// <summary>
	/// Read Only: The Drbg generators class name
	/// </summary>
	const std::string Name() override;

	/// <summary>
	/// Read Only: The recommended size of the nonce counter value in bytes
	/// </summary>
	const size_t NonceSize() override;

	/// <summary>
	/// Read/Write: Generating this amount or greater, triggers a re-seed
	/// </summary>
	size_t &ReseedThreshold() override;

	/// <summary>
	/// Read Only: The estimated security strength in bits.
	/// <para>This value depends both on the hash function output size, and the number of bits used to seed the generator.</para>
	/// </summary>
	const size_t SecurityStrength() override;

	//~~~Public Functions~~~//

	/// <summary>
	/// Generate a block of pseudo random bytes
	/// </summary>
	/// 
	/// <param name="Output">Output array filled with random bytes</param>
	/// 
	/// <returns>The number of bytes generated</returns>
	/// 
	/// <exception cref="Exception::CryptoGeneratorException">Thrown if the generator is not initialized, the output size is misaligned, 
	/// the maximum request size is exceeded, or if the maximum reseed requests are exceeded</exception>
	size_t Generate(std::vector<byte> &Output) override;

	/// <summary>
	/// Generate pseudo random bytes using offset and length parameters
	/// </summary>
	/// 
	/// <param name="Output">Output array filled with random bytes</param>
	/// <param name="OutOffset">The starting position within the Output array</param>
	/// <param name="Length">The number of bytes to generate</param>
	/// 
	/// <returns>The number of bytes generated</returns>
	/// 
	/// <exception cref="Exception::CryptoGeneratorException">Thrown if the generator is not initialized, the output size is misaligned, 
	/// the maximum request size is exceeded, or if the maximum reseed requests are exceeded</exception>
	size_t Generate(std::vector<byte> &Output, size_t OutOffset, size_t Length) override;

	/// <summary>
	/// Initialize the generator with a SymmetricKey structure containing the key, and optional nonce, and info string
	/// </summary>
	/// 
	/// <param name="GenParam">The SymmetricKey containing the generators keying material</param>
	/// 
	/// <exception cref="Exception::CryptoGeneratorException">Thrown if the seed is not a legal seed size</exception>
	void Initialize(ISymmetricKey &GenParam) override;

	/// <summary>
	/// Initialize the generator with a seed key.
	/// <para>Initializaes the genertor as a SHAKE instance</para>
	/// </summary>
	/// 
	/// <param name="Seed">The secret primary key array used to seed the generator; this initialization call creates a SHAKE implementation</param>
	/// 
	/// <exception cref="Exception::CryptoGeneratorException">Thrown if the seed is not a legal seed size</exception>
	void Initialize(const std::vector<byte> &Seed) override;

	/// <summary>
	/// Initialize the generator with the seed and nonce arrays.
	/// <para>Initializaes the genertor as a simple cSHAKE instance</para>
	/// </summary>
	/// 
	/// <param name="Seed">The secret primary key array used to seed the generator; see the LegalKeySizes property for accepted sizes</param>
	/// <param name="Nonce">The secret nonce value used as the customization string to initialize a simple cSHAKE variant</param>
	/// 
	/// <exception cref="Exception::CryptoGeneratorException">Thrown if the seed is not a legal seed size</exception>
	void Initialize(const std::vector<byte> &Seed, const std::vector<byte> &Nonce) override;

	/// <summary>
	/// Initialize the generator with a key, a nonce array, and an information string or nonce.
	/// <para>Initializaes the genertor as a cSHAKE instance</para>
	/// </summary>
	/// 
	/// <param name="Seed">The secret primary key array used to seed the generator; see the LegalKeySizes property for accepted sizes</param>
	/// <param name="Nonce">The secret nonce value used as the customization string to initialize a cSHAKE instance</param>
	/// <param name="Info">The info parameter used as a finction name or secret salt to initialize a cSHAKE instance</param>
	/// 
	/// <exception cref="Exception::CryptoGeneratorException">Thrown if the seed is not a legal seed size</exception>
	void Initialize(const std::vector<byte> &Seed, const std::vector<byte> &Nonce, const std::vector<byte> &Info) override;

	/// <summary>
	/// Update the generators keying material, used to refresh the state
	/// </summary>
	///
	/// <param name="Seed">The new seed value array</param>
	/// 
	/// <exception cref="Exception::CryptoGeneratorException">Thrown if the seed is too small</exception>
	void Update(const std::vector<byte> &Seed) override;

private:

	void Derive(const std::vector<byte> &Seed);
	void GenerateBlock(std::vector<byte> &Output, size_t OutOffset, size_t Length);
	void Scope();
};

NAMESPACE_DRBGEND
#endif
