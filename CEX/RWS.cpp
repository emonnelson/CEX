#include "RWS.h"
#include "CpuDetect.h"
#include "IntegerTools.h"
#include "KMAC.h"
#include "MemoryTools.h"
#include "Rijndael.h"
#include "SHAKE.h"

NAMESPACE_STREAM

// Quia Omni Tempore
// JGU, Feb 07 2020

using namespace Cipher::Block::RijndaelBase;
using Tools::IntegerTools;
using Mac::KMAC;
using Enumeration::KmacModes;
using Tools::MemoryTools;
using Tools::ParallelTools;
using Enumeration::ShakeModes;
using Enumeration::StreamCipherConvert;

class RWS::RwsState
{
public:

	SecureVector<uint> RoundKeys;
	SecureVector<byte> Associated;
	SecureVector<byte> Custom;
	SecureVector<byte> MacKey;
	SecureVector<byte> MacTag;
	SecureVector<byte> Name;
	std::vector<SymmetricKeySize> LegalKeySizes;
	std::vector<byte> Nonce;
	ulong Counter;
	uint Rounds;
	KmacModes Authenticator;
	ShakeModes Mode;
	bool IsAuthenticated;
	bool IsEncryption;
	bool IsInitialized;

	RwsState(bool Authenticate)
		:
		RoundKeys(0),
		Associated(0),
		Custom(0),
		MacKey(0),
		MacTag(0),
		Name(0),
		LegalKeySizes{
			SymmetricKeySize(IK256_SIZE, BLOCK_SIZE, INFO_SIZE),
			SymmetricKeySize(IK512_SIZE, BLOCK_SIZE, INFO_SIZE),
			SymmetricKeySize(IK1024_SIZE, BLOCK_SIZE, INFO_SIZE) },
		Nonce(BLOCK_SIZE, 0x00),
		Counter(0),
		Rounds(0),
		Authenticator(KmacModes::None),
		Mode(ShakeModes::None),
		IsAuthenticated(Authenticate),
		IsEncryption(false),
		IsInitialized(false)
	{
	}

	RwsState(SecureVector<byte> &State)
		:
		RoundKeys(0),
		Associated(0),
		Custom(0),
		MacKey(0),
		MacTag(0),
		Name(0),
		LegalKeySizes{
			SymmetricKeySize(IK256_SIZE, BLOCK_SIZE, INFO_SIZE),
			SymmetricKeySize(IK512_SIZE, BLOCK_SIZE, INFO_SIZE),
			SymmetricKeySize(IK1024_SIZE, BLOCK_SIZE, INFO_SIZE) },
		Nonce(BLOCK_SIZE, 0x00),
		Counter(0),
		Rounds(0),
		Authenticator(KmacModes::None),
		Mode(ShakeModes::None),
		IsAuthenticated(false),
		IsEncryption(false),
		IsInitialized(false)
	{
		DeSerialize(State);
	}

	~RwsState()
	{
		MemoryTools::Clear(RoundKeys, 0, RoundKeys.size() * sizeof(uint));
		MemoryTools::Clear(Associated, 0, Associated.size());
		MemoryTools::Clear(Custom, 0, Custom.size());
		MemoryTools::Clear(MacKey, 0, MacKey.size());
		MemoryTools::Clear(MacTag, 0, MacTag.size());
		MemoryTools::Clear(Name, 0, Name.size());
		MemoryTools::Clear(Nonce, 0, Nonce.size());
		LegalKeySizes.clear();
		Counter = 0;
		Rounds = 0;
		Authenticator = KmacModes::None;
		Mode = ShakeModes::None;
		IsAuthenticated = false;
		IsEncryption = false;
		IsInitialized = false;
	}

	void DeSerialize(SecureVector<byte> &SecureState)
	{
		size_t soff;
		ushort vlen;

		soff = 0;
		vlen = 0;

		MemoryTools::CopyToObject(SecureState, soff, &vlen, sizeof(ushort));
		RoundKeys.resize(vlen / sizeof(uint));
		soff += sizeof(ushort);
		MemoryTools::Copy(SecureState, soff, RoundKeys, 0, vlen);
		soff += vlen;

		MemoryTools::CopyToObject(SecureState, soff, &vlen, sizeof(ushort));
		Associated.resize(vlen);
		soff += sizeof(ushort);
		MemoryTools::Copy(SecureState, soff, Associated, 0, Associated.size());
		soff += vlen;

		MemoryTools::CopyToObject(SecureState, soff, &vlen, sizeof(ushort));
		Custom.resize(vlen);
		soff += sizeof(ushort);
		MemoryTools::Copy(SecureState, soff, Custom, 0, Custom.size());
		soff += vlen;

		MemoryTools::CopyToObject(SecureState, soff, &vlen, sizeof(ushort));
		MacKey.resize(vlen);
		soff += sizeof(ushort);
		MemoryTools::Copy(SecureState, soff, MacKey, 0, MacKey.size());
		soff += vlen;

		MemoryTools::CopyToObject(SecureState, soff, &vlen, sizeof(ushort));
		MacTag.resize(vlen);
		soff += sizeof(ushort);
		MemoryTools::Copy(SecureState, soff, MacTag, 0, MacTag.size());
		soff += vlen;

		MemoryTools::CopyToObject(SecureState, soff, &vlen, sizeof(ushort));
		Name.resize(vlen);
		soff += sizeof(ushort);
		MemoryTools::Copy(SecureState, soff, Name, 0, Name.size());
		soff += vlen;

		MemoryTools::CopyToObject(SecureState, soff, &vlen, sizeof(ushort));
		Nonce.resize(vlen);
		soff += sizeof(ushort);
		MemoryTools::Copy(SecureState, soff, Nonce, 0, Nonce.size());
		soff += vlen;

		MemoryTools::CopyToObject(SecureState, soff, &Counter, sizeof(ulong));
		soff += sizeof(ulong);
		MemoryTools::CopyToObject(SecureState, soff, &Rounds, sizeof(uint));
		soff += sizeof(uint);

		MemoryTools::CopyToObject(SecureState, soff, &Authenticator, sizeof(KmacModes));
		soff += sizeof(KmacModes);
		MemoryTools::CopyToObject(SecureState, soff, &Mode, sizeof(ShakeModes));
		soff += sizeof(ShakeModes);

		MemoryTools::CopyToObject(SecureState, soff, &IsAuthenticated, sizeof(bool));
		soff += sizeof(bool);
		MemoryTools::CopyToObject(SecureState, soff, &IsEncryption, sizeof(bool));
		soff += sizeof(bool);
		MemoryTools::CopyToObject(SecureState, soff, &IsInitialized, sizeof(bool));
	}

	void Reset()
	{
		MemoryTools::Clear(RoundKeys, 0, RoundKeys.size() * sizeof(uint));
		MemoryTools::Clear(Associated, 0, Associated.size());
		MemoryTools::Clear(Custom, 0, Custom.size());
		MemoryTools::Clear(MacKey, 0, MacKey.size());
		MemoryTools::Clear(MacTag, 0, MacTag.size());
		MemoryTools::Clear(Name, 0, Name.size());
		MemoryTools::Clear(Nonce, 0, Nonce.size());
		Counter = 0;
		Rounds = 0;
		IsEncryption = false;
		IsInitialized = false;
	}

	SecureVector<byte> Serialize()
	{
		const size_t STALEN = (RoundKeys.size() * sizeof(uint)) + Associated.size() + Custom.size() + MacKey.size() + MacTag.size() +
			Name.size() + Nonce.size() + sizeof(ulong) + sizeof(uint) + sizeof(KmacModes) + sizeof(ShakeModes) + (3 * sizeof(bool)) + (7 * sizeof(ushort));

		size_t soff;
		ushort vlen;
		SecureVector<byte> state(STALEN);

		soff = 0;
		vlen = static_cast<ushort>(RoundKeys.size() * sizeof(uint));
		MemoryTools::CopyFromObject(&vlen, state, soff, sizeof(ushort));
		soff += sizeof(ushort);
		MemoryTools::Copy(RoundKeys, 0, state, soff, static_cast<size_t>(vlen));
		soff += vlen;

		vlen = static_cast<ushort>(Associated.size());
		MemoryTools::CopyFromObject(&vlen, state, soff, sizeof(ushort));
		soff += sizeof(ushort);
		MemoryTools::Copy(Associated, 0, state, soff, Associated.size());
		soff += Associated.size();

		vlen = static_cast<ushort>(Custom.size());
		MemoryTools::CopyFromObject(&vlen, state, soff, sizeof(ushort));
		soff += sizeof(ushort);
		MemoryTools::Copy(Custom, 0, state, soff, Custom.size());
		soff += Custom.size();

		vlen = static_cast<ushort>(MacKey.size());
		MemoryTools::CopyFromObject(&vlen, state, soff, sizeof(ushort));
		soff += sizeof(ushort);
		MemoryTools::Copy(MacKey, 0, state, soff, MacKey.size());
		soff += MacKey.size();

		vlen = static_cast<ushort>(MacTag.size());
		MemoryTools::CopyFromObject(&vlen, state, soff, sizeof(ushort));
		soff += sizeof(ushort);
		MemoryTools::Copy(MacTag, 0, state, soff, MacTag.size());
		soff += MacTag.size();

		vlen = static_cast<ushort>(Name.size());
		MemoryTools::CopyFromObject(&vlen, state, soff, sizeof(ushort));
		soff += sizeof(ushort);
		MemoryTools::Copy(Name, 0, state, soff, Name.size());
		soff += Name.size();

		vlen = static_cast<ushort>(Nonce.size());
		MemoryTools::CopyFromObject(&vlen, state, soff, sizeof(ushort));
		soff += sizeof(ushort);
		MemoryTools::Copy(Nonce, 0, state, soff, Nonce.size());
		soff += Nonce.size();

		MemoryTools::CopyFromObject(&Counter, state, soff, sizeof(ulong));
		soff += sizeof(ulong);
		MemoryTools::CopyFromObject(&Rounds, state, soff, sizeof(uint));
		soff += sizeof(uint);

		MemoryTools::CopyFromObject(&Authenticator, state, soff, sizeof(KmacModes));
		soff += sizeof(KmacModes);
		MemoryTools::CopyFromObject(&Mode, state, soff, sizeof(ShakeModes));
		soff += sizeof(ShakeModes);

		MemoryTools::CopyFromObject(&IsAuthenticated, state, soff, sizeof(bool));
		soff += sizeof(bool);
		MemoryTools::CopyFromObject(&IsEncryption, state, soff, sizeof(bool));
		soff += sizeof(bool);
		MemoryTools::CopyFromObject(&IsInitialized, state, soff, sizeof(bool));

		return state;
	}
};

//~~~Constructor~~~//

RWS::RWS(bool Authenticate)
	:
	m_rwsState(new RwsState(Authenticate)),
	m_macAuthenticator(nullptr),
	m_parallelProfile(BLOCK_SIZE, true, STATE_PRECACHED, true)
{
}

RWS::RWS(SecureVector<byte> &State)
	:
	m_rwsState(State.size() > STATE_THRESHOLD ? new RwsState(State) : 
		throw CryptoSymmetricException(std::string("RWS"), std::string("Constructor"), std::string("The State array is invalid!"), ErrorCodes::InvalidKey)),
	m_macAuthenticator(m_rwsState->Authenticator == KmacModes::None ?
		nullptr :
		new KMAC(m_rwsState->Authenticator)),
	m_parallelProfile(BLOCK_SIZE, true, STATE_PRECACHED, true)
{
	if (m_rwsState->Authenticator != KmacModes::None)
	{
		// initialize the mac
		SymmetricKey kpm(m_rwsState->MacKey);
		m_macAuthenticator->Initialize(kpm);
	}
}

RWS::~RWS()
{
	if (m_macAuthenticator != nullptr)
	{
		m_macAuthenticator.reset(nullptr);
	}
}

//~~~Accessors~~~//

const StreamCiphers RWS::Enumeral()
{
	StreamAuthenticators auth;
	StreamCiphers tmpn;

	auth = IsAuthenticator() && m_macAuthenticator != nullptr ?
		static_cast<StreamAuthenticators>(m_macAuthenticator->Enumeral()) :
		StreamAuthenticators::None;

	tmpn = StreamCipherConvert::FromDescription(StreamCiphers::RWS, auth);

	return tmpn;
}

const bool RWS::IsAuthenticator()
{
	return m_rwsState->IsAuthenticated;
}

const bool RWS::IsEncryption()
{
	return m_rwsState->IsEncryption;
}

const bool RWS::IsInitialized()
{
	return m_rwsState->IsInitialized;
}

const bool RWS::IsParallel()
{
	return m_parallelProfile.IsParallel();
}

const std::vector<SymmetricKeySize> &RWS::LegalKeySizes()
{
	return m_rwsState->LegalKeySizes;
}

const std::string RWS::Name()
{
	std::string name;

	name = StreamCipherConvert::ToName(Enumeral());

	return name;
}

const std::vector<byte> RWS::Nonce()
{
	return m_rwsState->Nonce;
}

const size_t RWS::ParallelBlockSize()
{
	return m_parallelProfile.ParallelBlockSize();
}

ParallelOptions &RWS::ParallelProfile()
{
	return m_parallelProfile;
}

const std::vector<byte> RWS::Tag()
{
	if (m_rwsState->MacTag.size() == 0 || IsAuthenticator() == false)
	{
		throw CryptoSymmetricException(std::string("RCS"), std::string("Tag"), std::string("The cipher is not initialized for authentication or has not run!"), ErrorCodes::NotInitialized);
	}

	return SecureUnlock(m_rwsState->MacTag);
}

const void RWS::Tag(SecureVector<byte> &Output)
{
	if (m_rwsState->MacTag.size() == 0 || IsAuthenticator() == false)
	{
		throw CryptoSymmetricException(std::string("RCS"), std::string("Tag"), std::string("The cipher is not initialized for authentication or has not run!"), ErrorCodes::NotInitialized);
	}

	SecureCopy(m_rwsState->MacTag, 0, Output, 0, m_rwsState->MacTag.size());
}

const size_t RWS::TagSize()
{
	if (IsInitialized() == false)
	{
		throw CryptoSymmetricException(std::string("RWS"), std::string("TagSize"), std::string("The cipher has not been initialized!"), ErrorCodes::NotInitialized);
	}

	return IsAuthenticator() ? m_macAuthenticator->TagSize() : 0;
}

//~~~Public Functions~~~//

void RWS::Initialize(bool Encryption, ISymmetricKey &Parameters)
{
	size_t i;

	if (!SymmetricKeySize::Contains(LegalKeySizes(), Parameters.KeySizes().KeySize()))
	{
		throw CryptoSymmetricException(Name(), std::string("Initialize"), std::string("Invalid key size; key must be one of the LegalKeySizes in length."), ErrorCodes::InvalidKey);
	}
	if (Parameters.KeySizes().IVSize() != BLOCK_SIZE)
	{
		throw CryptoSymmetricException(Name(), std::string("Initialize"), std::string("Requires a nonce equal in size to the ciphers block size!"), ErrorCodes::InvalidNonce);
	}

	if (m_parallelProfile.IsParallel())
	{
		if (m_parallelProfile.IsParallel() && m_parallelProfile.ParallelBlockSize() < m_parallelProfile.ParallelMinimumSize() || m_parallelProfile.ParallelBlockSize() > m_parallelProfile.ParallelMaximumSize())
		{
			throw CryptoSymmetricException(Name(), std::string("Initialize"), std::string("The parallel block size is out of bounds!"), ErrorCodes::InvalidSize);
		}
		if (m_parallelProfile.IsParallel() && m_parallelProfile.ParallelBlockSize() % m_parallelProfile.ParallelMinimumSize() != 0)
		{
			throw CryptoSymmetricException(Name(), std::string("Initialize"), std::string("The parallel block size must be evenly aligned to the ParallelMinimumSize!"), ErrorCodes::InvalidParam);
		}
	}

	// reset for a new key
	if (IsInitialized() == true)
	{
		Reset();
	}

	// cipher key size determines key expansion function and Mac generator type; 256, 512, or 1024-bit
	m_rwsState->Mode = (Parameters.KeySizes().KeySize() == IK1024_SIZE) ?
		ShakeModes::SHAKE1024 : 
		(Parameters.KeySizes().KeySize() == IK512_SIZE) ?
			ShakeModes::SHAKE512 :
			ShakeModes::SHAKE256;

	if (m_rwsState->IsAuthenticated)
	{
		m_rwsState->Authenticator = (Parameters.KeySizes().KeySize() == IK1024_SIZE) ?
			KmacModes::KMAC1024 :
			(Parameters.KeySizes().KeySize() == IK512_SIZE) ?
			KmacModes::KMAC512 :
			KmacModes::KMAC256;

		m_macAuthenticator.reset(new KMAC(m_rwsState->Authenticator));
	}

	// set the number of rounds
	m_rwsState->Rounds = (Parameters.KeySizes().KeySize() == IK1024_SIZE) ?
		RK1024_COUNT :
		(Parameters.KeySizes().KeySize() == IK512_SIZE) ?
			RK512_COUNT :
			RK256_COUNT;

	// set the initial processed-bytes count to zero
	m_rwsState->Counter = 0;

	// store the customization string
	if (Parameters.KeySizes().InfoSize() != 0)
	{
		m_rwsState->Custom.resize(Parameters.KeySizes().InfoSize());
		// copy the user defined string to the customization parameter
		MemoryTools::Copy(Parameters.Info(), 0, m_rwsState->Custom, 0, Parameters.KeySizes().InfoSize());
	}

	// create the cSHAKE name string
	std::string tmpn = Name();
	// add mac counter, key-size bits, and algorithm name to name string
	m_rwsState->Name.resize(sizeof(ulong) + sizeof(ushort) + tmpn.size());
	// mac counter is always first 8 bytes
	IntegerTools::Le64ToBytes(m_rwsState->Counter, m_rwsState->Name, 0);
	// add the cipher key size in bits as an unsigned short integer
	ushort kbits = static_cast<ushort>(Parameters.KeySizes().KeySize() * 8);
	IntegerTools::Le16ToBytes(kbits, m_rwsState->Name, sizeof(ulong));
	// copy the name string to state
	MemoryTools::CopyFromObject(tmpn.data(), m_rwsState->Name, sizeof(ulong) + sizeof(ushort), tmpn.size());

	// copy the nonce to state
	MemoryTools::Copy(Parameters.IV(), 0, m_rwsState->Nonce, 0, BLOCK_SIZE);

	// initialize cSHAKE with k,c,n
	Kdf::SHAKE gen(m_rwsState->Mode);
	gen.Initialize(Parameters.SecureKey(), m_rwsState->Custom, m_rwsState->Name);

	// size the round key array
	const size_t RNKLEN = (BLOCK_SIZE / sizeof(uint)) * static_cast<size_t>(m_rwsState->Rounds + 1UL);
	m_rwsState->RoundKeys.resize(RNKLEN);
	// generate the round keys to a temporary byte array
	SecureVector<byte> tmpr(RNKLEN * sizeof(uint));
	// generate the ciphers round-keys
	gen.Generate(tmpr);

	// realign in big endian format for [future] AES-NI test vectors; RWS will be the fallback to the AES-NI implementation
	for (i = 0; i < tmpr.size() / sizeof(uint); ++i)
	{
		m_rwsState->RoundKeys[i] = IntegerTools::BeBytesTo32(tmpr, i * sizeof(uint));
	}

	MemoryTools::Clear(tmpr, 0, tmpr.size());

	if (IsAuthenticator()) 
	{
		// generate the mac key
		SymmetricKeySize ks = m_macAuthenticator->LegalKeySizes()[1];
		SecureVector<byte> mack(ks.KeySize());
		gen.Generate(mack);
		// initialize the mac
		SymmetricKey kpm(mack);
		m_macAuthenticator->Initialize(kpm);
		// store the key
		m_rwsState->MacKey.resize(mack.size());
		SecureMove(mack, 0, m_rwsState->MacKey, 0, mack.size());
		m_rwsState->MacTag.resize(m_macAuthenticator->TagSize());
	}

	m_rwsState->IsEncryption = Encryption;
	m_rwsState->IsInitialized = true;
}

void RWS::ParallelMaxDegree(size_t Degree)
{
	if (Degree == 0 || Degree % 2 != 0 || Degree > m_parallelProfile.ProcessorCount())
	{
		throw CryptoSymmetricException(Name(), std::string("ParallelMaxDegree"), std::string("Degree setting is invalid!"), ErrorCodes::NotSupported);
	}

	m_parallelProfile.SetMaxDegree(Degree);
}

void RWS::SetAssociatedData(const std::vector<byte> &Input, size_t Offset, size_t Length)
{
	if (IsInitialized() == false)
	{
		throw CryptoSymmetricException(Name(), std::string("SetAssociatedData"), std::string("The cipher has not been initialized!"), ErrorCodes::NotInitialized);
	}
	if (m_macAuthenticator == nullptr)
	{
		throw CryptoSymmetricException(Name(), std::string("SetAssociatedData"), std::string("The cipher has not been configured for authentication!"), ErrorCodes::IllegalOperation);
	}

	// store the associated data
	m_rwsState->Associated.resize(Length);
	MemoryTools::Copy(Input, Offset, m_rwsState->Associated, 0, Length);
}

void RWS::Transform(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset, size_t Length)
{
	CEXASSERT(IsInitialized(), "The cipher mode has not been initialized!");
	CEXASSERT(IntegerTools::Min(Input.size() - InOffset, Output.size() - OutOffset) >= Length, "The data arrays are smaller than the block-size!");

	if (IsEncryption() == true)
	{
		if (IsAuthenticator() == true)
		{
			if (Output.size() < Length + OutOffset + m_macAuthenticator->TagSize())
			{
				throw CryptoSymmetricException(Name(), std::string("Transform"), std::string("The vector is not long enough to add the MAC code!"), ErrorCodes::InvalidSize);
			}

			// add the starting position of the nonce
			m_macAuthenticator->Update(m_rwsState->Nonce, 0, BLOCK_SIZE);
			// encrypt the stream
			Process(Input, InOffset, Output, OutOffset, Length);
			// update the mac with the ciphertext
			m_macAuthenticator->Update(Output, OutOffset, Length);
			// update the processed bytes counter
			m_rwsState->Counter += Length;
			// finalize the mac and copy the tag to the end of the output stream
			Finalize(m_rwsState, m_macAuthenticator);
			MemoryTools::Copy(m_rwsState->MacTag, 0, Output, OutOffset + Length, m_rwsState->MacTag.size());
		}
		else
		{
			// encrypt the stream
			Process(Input, InOffset, Output, OutOffset, Length);
		}
	}
	else
	{
		if (IsAuthenticator())
		{
			// add the starting position of the nonce
			m_macAuthenticator->Update(m_rwsState->Nonce, 0, BLOCK_SIZE);
			// update the mac with the ciphertext
			m_macAuthenticator->Update(Input, InOffset, Length);
			// update the processed bytes counter
			m_rwsState->Counter += Length;
			// finalize the mac and verify
			Finalize(m_rwsState, m_macAuthenticator);

			if (!IntegerTools::Compare(Input, InOffset + Length, m_rwsState->MacTag, 0, m_rwsState->MacTag.size()))
			{
				throw CryptoAuthenticationFailure(Name(), std::string("Transform"), std::string("The authentication tag does not match!"), ErrorCodes::AuthenticationFailure);
			}
		}

		// decrypt the stream
		Process(Input, InOffset, Output, OutOffset, Length);
	}
}

//~~~Private Functions~~~//

void RWS::Finalize(std::unique_ptr<RwsState> &State, std::unique_ptr<IMac> &Authenticator)
{
	std::vector<byte> mctr(sizeof(ulong));
	ulong mlen;

	// 1.0c: add the total number of bytes processed by the mac, including this terminating string
	mlen = State->Counter + State->Nonce.size() + State->Associated.size() + mctr.size();
	IntegerTools::LeIncrease8(mctr, mlen);

	// 1.0c: add the associated data to the mac
	if (State->Associated.size() != 0)
	{
		Authenticator->Update(SecureUnlock(State->Associated), 0, State->Associated.size());
		// clear the associated data, reset for each transformation, 
		// assignable with a call to SetAssociatedData before each transform call
		SecureClear(State->Associated);
	}

	// add the termination string to the mac
	Authenticator->Update(mctr, 0, mctr.size());

	// 1.0b: finalize the mac code to state
	Authenticator->Finalize(State->MacTag, 0);
}

void RWS::Generate(std::vector<byte> &Output, size_t OutOffset, size_t Length, std::vector<byte> &Counter)
{
	size_t bctr;

	bctr = 0;

	// Note: The counter length passed into LEIncrement, only processes the first 16 bytes
	// as the full counter length. This is because this cipher is not expected to encrypt
	// more that 2^128 bytes of data with a single key.

#if defined(CEX_HAS_AVX512)

	const size_t AVX512BLK = 16 * BLOCK_SIZE;

	if (Length >= AVX512BLK)
	{
		const size_t PBKALN = Length - (Length % AVX512BLK);
		std::vector<byte> tmpc(AVX512BLK);

		// stagger counters and process 8 blocks with avx512
		while (bctr != PBKALN)
		{
			MemoryTools::Copy(Counter, 0, tmpc, 0, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 64, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 128, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 192, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 256, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 320, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 384, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 448, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 512, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 576, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 640, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 704, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 768, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 832, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 896, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 960, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			Transform8192(tmpc, 0, Output, OutOffset + bctr);
			bctr += AVX512BLK;
		}
	}

#elif defined(CEX_HAS_AVX2)

	const size_t AVX2BLK = 8 * BLOCK_SIZE;

	if (Length >= AVX2BLK)
	{
		const size_t PBKALN = Length - (Length % AVX2BLK);
		std::vector<byte> tmpc(AVX2BLK);

		// stagger counters and process 8 blocks with avx2
		while (bctr != PBKALN)
		{
			MemoryTools::Copy(Counter, 0, tmpc, 0, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 64, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 128, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 192, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 256, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 320, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 384, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 448, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			Transform4096(tmpc, 0, Output, OutOffset + bctr);
			bctr += AVX2BLK;
		}
	}

#elif defined(CEX_HAS_AVX)

	const size_t AVXBLK = 4 * BLOCK_SIZE;

	if (Length >= AVXBLK)
	{
		const size_t PBKALN = Length - (Length % AVXBLK);
		std::vector<byte> tmpc(AVXBLK);

		// 4 blocks with avx
		while (bctr != PBKALN)
		{
			MemoryTools::Copy(Counter, 0, tmpc, 0, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 64, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 128, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 192, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			Transform2048(tmpc, 0, Output, OutOffset + bctr);
			bctr += AVXBLK;
		}
	}

#endif

	const size_t BLKALN = Length - (Length % BLOCK_SIZE);

	while (bctr != BLKALN)
	{
		Transform512(Counter, 0, Output, OutOffset + bctr);
		IntegerTools::LeIncrement(Counter, 16);
		bctr += BLOCK_SIZE;
	}

	if (bctr != Length)
	{
		std::vector<byte> otp(BLOCK_SIZE);
		Transform512(Counter, 0, otp, 0);
		IntegerTools::LeIncrement(Counter, 16);
		const size_t RMDLEN = Length % BLOCK_SIZE;
		MemoryTools::Copy(otp, 0, Output, OutOffset + (Length - RMDLEN), RMDLEN);
	}
}

CEX_OPTIMIZE_IGNORE
void RWS::PrefetchSbox()
{
	// timing defence: pre-load sbox into l1 cache
	MemoryTools::PrefetchL1(SBox, 0, SBox.size());
}
CEX_OPTIMIZE_RESUME

void RWS::Process(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset, size_t Length)
{
	size_t i;

	const size_t PRLBLK = m_parallelProfile.ParallelBlockSize();

	if (m_parallelProfile.IsParallel() && Length >= PRLBLK)
	{
		const size_t BLKCNT = Length / PRLBLK;

		for (i = 0; i < BLKCNT; ++i)
		{
			ProcessParallel(Input, InOffset + (i * PRLBLK), Output, OutOffset + (i * PRLBLK), PRLBLK);
		}

		const size_t RMDLEN = Length - (PRLBLK * BLKCNT);

		if (RMDLEN != 0)
		{
			const size_t BLKOFT = (PRLBLK * BLKCNT);
			ProcessSequential(Input, InOffset + BLKOFT, Output, OutOffset + BLKOFT, RMDLEN);
		}
	}
	else
	{
		ProcessSequential(Input, InOffset, Output, OutOffset, Length);
	}
}

void RWS::ProcessParallel(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset, size_t Length)
{
	const size_t OUTLEN = Output.size() - OutOffset < Length ? Output.size() - OutOffset : Length;
	const size_t CNKLEN = m_parallelProfile.ParallelBlockSize() / m_parallelProfile.ParallelMaxDegree();
	const size_t CTRLEN = (CNKLEN / BLOCK_SIZE);
	std::vector<byte> tmpc(BLOCK_SIZE);

	ParallelTools::ParallelFor(0, m_parallelProfile.ParallelMaxDegree(), [this, &Input, InOffset, &Output, OutOffset, &tmpc, CNKLEN, CTRLEN](size_t i)
	{
		// thread level counter
		std::vector<byte> thdc(BLOCK_SIZE);
		// offset counter by chunk size / block size  
		IntegerTools::LeIncrease8(m_rwsState->Nonce, thdc, static_cast<uint>(CTRLEN * i));
		const size_t STMPOS = i * CNKLEN;
		// generate random at output offset
		this->Generate(Output, OutOffset + STMPOS, CNKLEN, thdc);
		// xor with input at offsets
		MemoryTools::XOR(Input, InOffset + STMPOS, Output, OutOffset + STMPOS, CNKLEN);

		// store last counter
		if (i == m_parallelProfile.ParallelMaxDegree() - 1)
		{
			MemoryTools::Copy(thdc, 0, tmpc, 0, BLOCK_SIZE);
		}
	});

	// copy last counter to class variable
	MemoryTools::Copy(tmpc, 0, m_rwsState->Nonce, 0, BLOCK_SIZE);

	// last block processing
	const size_t ALNLEN = CNKLEN * m_parallelProfile.ParallelMaxDegree();

	if (ALNLEN < OUTLEN)
	{
		const size_t FNLLEN = OUTLEN - ALNLEN;
		InOffset += ALNLEN;
		OutOffset += ALNLEN;

		Generate(Output, OutOffset, FNLLEN, m_rwsState->Nonce);

		for (size_t i = 0; i < FNLLEN; ++i)
		{
			Output[OutOffset + i] ^= Input[InOffset + i];
		}
	}
}

void RWS::ProcessSequential(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset, size_t Length)
{
	// get block aligned
	const size_t ALNLEN = Length - (Length % BLOCK_SIZE);
	size_t i;

	// generate random
	Generate(Output, OutOffset, Length, m_rwsState->Nonce);

	if (ALNLEN != 0)
	{
		MemoryTools::XOR(Input, InOffset, Output, OutOffset, ALNLEN);
	}

	// get the remaining bytes
	if (ALNLEN != Length)
	{
		for (i = ALNLEN; i < Length; ++i)
		{
			Output[i + OutOffset] ^= Input[i + InOffset];
		}
	}
}

void RWS::Reset()
{
	m_rwsState->Reset();

	if (IsAuthenticator())
	{
		m_macAuthenticator->Reset();
	}

	m_parallelProfile.Calculate(m_parallelProfile.IsParallel(), m_parallelProfile.ParallelBlockSize(), m_parallelProfile.ParallelMaxDegree());
}

SecureVector<byte> RWS::Serialize()
{
	SecureVector<byte> tmps = m_rwsState->Serialize();

	return tmps;
}

void RWS::Transform512(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
{
	SecureVector<byte> state(BLOCK_SIZE, 0x00);
	size_t i;

	MemoryTools::Copy(Input, InOffset, state, 0, BLOCK_SIZE);
	KeyAddition(state, m_rwsState->RoundKeys, 0);

	// pre-load the s-box into L1 cache
#if defined(CEX_PREFETCH_RIJNDAEL_TABLES)
	PrefetchSbox();
#endif

	for (i = 1; i < m_rwsState->Rounds; ++i)
	{
		Substitution(state);
		ShiftRows512(state);
		MixColumns(state);
		KeyAddition(state, m_rwsState->RoundKeys, (i << 4UL));
	}

	Substitution(state);
	ShiftRows512(state);
	KeyAddition(state, m_rwsState->RoundKeys, static_cast<size_t>(m_rwsState->Rounds) << 4UL);

	MemoryTools::Copy(state, 0, Output, OutOffset, BLOCK_SIZE);
}

void RWS::Transform2048(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
{
	Transform512(Input, InOffset, Output, OutOffset);
	Transform512(Input, InOffset + 64, Output, OutOffset + 64);
	Transform512(Input, InOffset + 128, Output, OutOffset + 128);
	Transform512(Input, InOffset + 192, Output, OutOffset + 192);
}

void RWS::Transform4096(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
{
	Transform2048(Input, InOffset, Output, OutOffset);
	Transform2048(Input, InOffset + 256, Output, OutOffset + 256);
}

void RWS::Transform8192(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
{
	Transform4096(Input, InOffset, Output, OutOffset);
	Transform4096(Input, InOffset + 512, Output, OutOffset + 512);
}

NAMESPACE_STREAMEND
