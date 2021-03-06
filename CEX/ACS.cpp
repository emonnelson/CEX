#include "ACS.h"
#include "CpuDetect.h"
#include "IntegerTools.h"
#include "KMAC.h"
#include "MemoryTools.h"
#include "SHAKE.h"
#include "StreamAuthenticators.h"
#include <wmmintrin.h>

NAMESPACE_STREAM

using Tools::IntegerTools;
using Mac::KMAC;
using Tools::MemoryTools;
using Tools::ParallelTools;
using Enumeration::ShakeModes;
using Enumeration::StreamAuthenticators;
using Enumeration::StreamCipherConvert;

const __m128i ACS::BLEND_MASK = _mm_set_epi32(0x80000000UL, 0x80800000UL, 0x80800000UL, 0x80808000UL);
const __m128i ACS::SHIFT_MASK = { 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13, 2, 3 };

class ACS::AcsState
{
public:

	std::vector<__m128i> RoundKeys;
	SecureVector<byte> Associated;
	SecureVector<byte> Custom;
	SecureVector<byte> MacKey;
	SecureVector<byte> MacTag;
	SecureVector<byte> Name;
	std::vector<SymmetricKeySize> LegalKeySizes;
	std::vector<byte> Nonce;
	ulong Counter;
	ushort Rounds;
	KmacModes Authenticator;
	ShakeModes Mode;
	bool IsAuthenticated;
	bool IsEncryption;
	bool Initialized;

	AcsState(bool Authenticate)
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
		Authenticator(Authenticator),
		Mode(ShakeModes::None),
		IsAuthenticated(Authenticate),
		IsEncryption(false),
		Initialized(false)
	{
	}

	AcsState(SecureVector<byte> &State)
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
		Initialized(false)
	{
		DeSerialize(State);
	}

	~AcsState()
	{
		MemoryTools::Clear(RoundKeys, 0, RoundKeys.size() * sizeof(__m128i));
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
		Initialized = false;
	}

	void DeSerialize(SecureVector<byte> &SecureState)
	{
		size_t soff;
		ushort vlen;

		soff = 0;
		vlen = 0;

		MemoryTools::CopyToObject(SecureState, soff, &vlen, sizeof(ushort));
		RoundKeys.resize(vlen / sizeof(__m128i));
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
		MemoryTools::CopyToObject(SecureState, soff, &Rounds, sizeof(ushort));
		soff += sizeof(ushort);

		MemoryTools::CopyToObject(SecureState, soff, &Authenticator, sizeof(KmacModes));
		soff += sizeof(KmacModes);
		MemoryTools::CopyToObject(SecureState, soff, &Mode, sizeof(ShakeModes));
		soff += sizeof(ShakeModes);

		MemoryTools::CopyToObject(SecureState, soff, &IsAuthenticated, sizeof(bool));
		soff += sizeof(bool);
		MemoryTools::CopyToObject(SecureState, soff, &IsEncryption, sizeof(bool));
		soff += sizeof(bool);
		MemoryTools::CopyToObject(SecureState, soff, &Initialized, sizeof(bool));
	}

	void Reset()
	{
		MemoryTools::Clear(RoundKeys, 0, RoundKeys.size() * sizeof(__m128i));
		MemoryTools::Clear(Associated, 0, Associated.size());
		MemoryTools::Clear(Custom, 0, Custom.size());
		MemoryTools::Clear(MacKey, 0, MacKey.size());
		MemoryTools::Clear(MacTag, 0, MacTag.size());
		MemoryTools::Clear(Name, 0, Name.size());
		MemoryTools::Clear(Nonce, 0, Nonce.size());
		Counter = 0;
		Rounds = 0;
		IsEncryption = false;
		Initialized = false;
	}

	SecureVector<byte> Serialize()
	{
		const size_t STALEN = (RoundKeys.size() * sizeof(__m128i)) + Associated.size() + Custom.size() + MacKey.size() + MacTag.size() +
			Name.size() + Nonce.size() + sizeof(ulong) + sizeof(ushort) + sizeof(KmacModes) + sizeof(ShakeModes) + (3 * sizeof(bool)) + (7 * sizeof(ushort));

		size_t soff;
		ushort vlen;
		SecureVector<byte> state(STALEN);

		soff = 0;
		vlen = static_cast<ushort>(RoundKeys.size() * sizeof(__m128i));
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
		MemoryTools::CopyFromObject(&Rounds, state, soff, sizeof(ushort));
		soff += sizeof(ushort);

		MemoryTools::CopyFromObject(&Authenticator, state, soff, sizeof(KmacModes));
		soff += sizeof(KmacModes);
		MemoryTools::CopyFromObject(&Mode, state, soff, sizeof(ShakeModes));
		soff += sizeof(ShakeModes);

		MemoryTools::CopyFromObject(&IsAuthenticated, state, soff, sizeof(bool));
		soff += sizeof(bool);
		MemoryTools::CopyFromObject(&IsEncryption, state, soff, sizeof(bool));
		soff += sizeof(bool);
		MemoryTools::CopyFromObject(&Initialized, state, soff, sizeof(bool));

		return state;
	}
};

//~~~Constructor~~~//

ACS::ACS(bool Authenticate)
	:
	m_acsState(new AcsState(Authenticate)),
	m_macAuthenticator(nullptr),
	m_parallelProfile(BLOCK_SIZE, true, STATE_PRECACHED, true)
{
#if !defined(CEX_AVX_INTRINSICS)
	throw CryptoSymmetricException(StreamCipherConvert::ToName(StreamCiphers::RCS), std::string("Constructor"), std::string("AVX is not supported on this system!"), ErrorCodes::NotSupported);
#endif
}

ACS::ACS(SecureVector<byte> &State)
	:
	m_acsState(State.size() > STATE_THRESHOLD ? new AcsState(State) :
		throw CryptoSymmetricException(std::string("ACS"), std::string("Constructor"), std::string("The State array is invalid!"), ErrorCodes::InvalidKey)),
	m_macAuthenticator(m_acsState->Authenticator == KmacModes::None ?
		nullptr :
		new KMAC(m_acsState->Authenticator)),
	m_parallelProfile(BLOCK_SIZE, true, STATE_PRECACHED, true)
{
#if !defined(CEX_AVX_INTRINSICS)
	throw CryptoSymmetricException(StreamCipherConvert::ToName(StreamCiphers::RCS), std::string("Constructor"), std::string("AVX is not supported on this system!"), ErrorCodes::NotSupported);
#endif

	if (m_acsState->Authenticator != KmacModes::None)
	{
		// initialize the mac
		SymmetricKey kpm(m_acsState->MacKey);
		m_macAuthenticator->Initialize(kpm);
	}
}

ACS::~ACS()
{
	if (m_macAuthenticator != nullptr)
	{
		m_macAuthenticator.reset(nullptr);
	}
}

//~~~Accessors~~~//

const StreamCiphers ACS::Enumeral()
{
	StreamAuthenticators auth;
	StreamCiphers tmpn;

	auth = IsAuthenticator() && m_macAuthenticator != nullptr ?
		static_cast<StreamAuthenticators>(m_macAuthenticator->Enumeral()) :
		StreamAuthenticators::None;

	tmpn = StreamCipherConvert::FromDescription(StreamCiphers::RCS, auth);

	return tmpn;
}

const bool ACS::IsAuthenticator()
{
	return m_acsState->IsAuthenticated;
}

const bool ACS::IsEncryption()
{
	return m_acsState->IsEncryption;
}

const bool ACS::IsInitialized()
{
	return m_acsState->Initialized;
}

const bool ACS::IsParallel()
{
	return m_parallelProfile.IsParallel();
}

const std::vector<SymmetricKeySize> &ACS::LegalKeySizes()
{
	return m_acsState->LegalKeySizes;
}

const std::string ACS::Name()
{
	std::string name;

	name = StreamCipherConvert::ToName(Enumeral());

	return name;
}

const std::vector<byte> ACS::Nonce()
{
	return m_acsState->Nonce;
}

const size_t ACS::ParallelBlockSize()
{
	return m_parallelProfile.ParallelBlockSize();
}

ParallelOptions &ACS::ParallelProfile()
{
	return m_parallelProfile;
}

const std::vector<byte> ACS::Tag()
{
	if (m_acsState->MacTag.size() == 0 || IsAuthenticator() == false)
	{
		throw CryptoSymmetricException(std::string("RCS"), std::string("Tag"), std::string("The cipher is not initialized for authentication or has not run!"), ErrorCodes::NotInitialized);
	}

	return SecureUnlock(m_acsState->MacTag);
}

const void ACS::Tag(SecureVector<byte> &Output)
{
	if (m_acsState->MacTag.size() == 0 || IsAuthenticator() == false)
	{
		throw CryptoSymmetricException(std::string("RCS"), std::string("Tag"), std::string("The cipher is not initialized for authentication or has not run!"), ErrorCodes::NotInitialized);
	}

	SecureCopy(m_acsState->MacTag, 0, Output, 0, m_acsState->MacTag.size());
}

const size_t ACS::TagSize()
{
	if (IsInitialized() == false)
	{
		throw CryptoSymmetricException(std::string("RCS"), std::string("TagSize"), std::string("The cipher has not been initialized!"), ErrorCodes::NotInitialized);
	}

	return IsAuthenticator() ? m_macAuthenticator->TagSize() : 0;
}

//~~~Public Functions~~~//

void ACS::Initialize(bool Encryption, ISymmetricKey &Parameters)
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

	// set the initial processed-bytes count to one
	m_acsState->Counter = 1;

	// set the number of rounds -v1.0d
	m_acsState->Rounds = (Parameters.KeySizes().KeySize() == IK256_SIZE) ?
		RK256_COUNT :
		(Parameters.KeySizes().KeySize() == IK512_SIZE) ?
			RK512_COUNT :
			RK1024_COUNT;

	if (m_acsState->IsAuthenticated)
	{
		m_acsState->Authenticator = (Parameters.KeySizes().KeySize() == IK1024_SIZE) ?
			KmacModes::KMAC1024 :
			(Parameters.KeySizes().KeySize() == IK512_SIZE) ?
			KmacModes::KMAC512 :
			KmacModes::KMAC256;

		m_macAuthenticator.reset(new KMAC(m_acsState->Authenticator));
	}

	// store the customization string -v1.0d
	if (Parameters.KeySizes().InfoSize() != 0)
	{
		m_acsState->Custom.resize(Parameters.KeySizes().InfoSize());
		// copy the user defined string to the customization parameter
		MemoryTools::Copy(Parameters.Info(), 0, m_acsState->Custom, 0, Parameters.KeySizes().InfoSize());
	}

	// create the cSHAKE name string
	std::string tmpn = Name();
	// add mac counter, key-size bits, and algorithm name to name string
	m_acsState->Name.resize(sizeof(ulong) + sizeof(ushort) + tmpn.size());
	// mac counter is always first 8 bytes of name
	IntegerTools::Le64ToBytes(m_acsState->Counter, m_acsState->Name, 0);
	// add the cipher key size in bits as an unsigned 16-bit integer
	ushort kbits = static_cast<ushort>(Parameters.KeySizes().KeySize() * 8);
	IntegerTools::Le16ToBytes(kbits, m_acsState->Name, sizeof(ulong));
	// copy the name string to state
	MemoryTools::CopyFromObject(tmpn.data(), m_acsState->Name, sizeof(ulong) + sizeof(ushort), tmpn.size());

	// copy the nonce to state
	MemoryTools::Copy(Parameters.IV(), 0, m_acsState->Nonce, 0, BLOCK_SIZE);

	// cipher key size determines key expansion function and Mac generator type; 256 or 512-bit
	m_acsState->Mode = (Parameters.KeySizes().KeySize() == IK512_SIZE) ?
		ShakeModes::SHAKE512 : 
		(Parameters.KeySizes().KeySize() == IK256_SIZE) ?
			ShakeModes::SHAKE256 : 
			ShakeModes::SHAKE1024;

	// initialize the generator
	Kdf::SHAKE gen(m_acsState->Mode);
	// key with cSHAKE(k,c,n)
	gen.Initialize(Parameters.SecureKey(), m_acsState->Custom, m_acsState->Name);

	// calculate the size of the round-key array
	const size_t RNKLEN = static_cast<size_t>(BLOCK_SIZE / sizeof(__m128i)) * static_cast<size_t>(m_acsState->Rounds + 1UL);
	m_acsState->RoundKeys.resize(RNKLEN);
	SecureVector<byte> tmpr(RNKLEN * sizeof(__m128i));
	// generate the cipher round-keys
	gen.Generate(tmpr);

	// copy p-rand bytes to round keys
	for (i = 0; i < RNKLEN; ++i)
	{
		m_acsState->RoundKeys[i] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&tmpr[i * sizeof(__m128i)]));
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
		m_acsState->MacKey.resize(mack.size());
		SecureMove(mack, 0, m_acsState->MacKey, 0, mack.size());
		m_acsState->MacTag.resize(m_macAuthenticator->TagSize());
	}

	m_acsState->IsEncryption = Encryption;
	m_acsState->Initialized = true;
}

void ACS::ParallelMaxDegree(size_t Degree)
{
	if (Degree == 0 || Degree % 2 != 0 || Degree > m_parallelProfile.ProcessorCount())
	{
		throw CryptoSymmetricException(Name(), std::string("ParallelMaxDegree"), std::string("Degree setting is invalid!"), ErrorCodes::NotSupported);
	}

	m_parallelProfile.SetMaxDegree(Degree);
}

void ACS::SetAssociatedData(const std::vector<byte> &Input, size_t Offset, size_t Length)
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
	m_acsState->Associated.resize(Length);
	MemoryTools::Copy(Input, Offset, m_acsState->Associated, 0, Length);
}

void ACS::Transform(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset, size_t Length)
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
			m_macAuthenticator->Update(m_acsState->Nonce, 0, BLOCK_SIZE);
			// encrypt the stream
			Process(Input, InOffset, Output, OutOffset, Length);
			// update the mac with the ciphertext
			m_macAuthenticator->Update(Output, OutOffset, Length);
			// update the processed bytes counter
			m_acsState->Counter += Length;
			// finalize the mac and copy the tag to the end of the output stream
			Finalize(m_acsState, m_macAuthenticator);
			MemoryTools::Copy(m_acsState->MacTag, 0, Output, OutOffset + Length, m_acsState->MacTag.size());
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
			m_macAuthenticator->Update(m_acsState->Nonce, 0, BLOCK_SIZE);
			// update the mac with the ciphertext
			m_macAuthenticator->Update(Input, InOffset, Length);
			// update the processed bytes counter
			m_acsState->Counter += Length;
			// finalize the mac and verify
			Finalize(m_acsState, m_macAuthenticator);

			if (!IntegerTools::Compare(Input, InOffset + Length, m_acsState->MacTag, 0, m_acsState->MacTag.size()))
			{
				throw CryptoAuthenticationFailure(Name(), std::string("Transform"), std::string("The authentication tag does not match!"), ErrorCodes::AuthenticationFailure);
			}
		}

		// decrypt the stream
		Process(Input, InOffset, Output, OutOffset, Length);
	}
}

//~~~Private Functions~~~//

void ACS::Finalize(std::unique_ptr<AcsState> &State, std::unique_ptr<IMac> &Authenticator)
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

	// 1.0e: finalize the mac code to state
	Authenticator->Finalize(State->MacTag, 0);
}

void ACS::Generate(std::vector<byte> &Output, size_t OutOffset, size_t Length, std::vector<byte> &Counter)
{
	size_t bctr;

	bctr = 0;

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
			MemoryTools::Copy(Counter, 0, tmpc, 32, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 64, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 96, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 128, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 160, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 192, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 224, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 256, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 288, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 320, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 352, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 384, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 416, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 448, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 480, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			Transform4096(tmpc, 0, Output, OutOffset + bctr);
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
			MemoryTools::Copy(Counter, 0, tmpc, 32, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 64, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 96, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 128, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 160, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 192, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 224, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			Transform2048(tmpc, 0, Output, OutOffset + bctr);
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
			MemoryTools::Copy(Counter, 0, tmpc, 32, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 64, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			MemoryTools::Copy(Counter, 0, tmpc, 96, BLOCK_SIZE);
			IntegerTools::LeIncrement(Counter, 16);
			Transform1024(tmpc, 0, Output, OutOffset + bctr);
			bctr += AVXBLK;
		}
	}

#endif

	const size_t BLKALN = Length - (Length % BLOCK_SIZE);
	while (bctr != BLKALN)
	{
		Transform256(Counter, 0, Output, OutOffset + bctr);
		IntegerTools::LeIncrement(Counter, 16);
		bctr += BLOCK_SIZE;
	}

	if (bctr != Length)
	{
		std::vector<byte> otp(BLOCK_SIZE);
		Transform256(Counter, 0, otp, 0);
		IntegerTools::LeIncrement(Counter, 16);
		const size_t RMDLEN = Length % BLOCK_SIZE;
		MemoryTools::Copy(otp, 0, Output, OutOffset + (Length - RMDLEN), RMDLEN);
	}
}

void ACS::Process(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset, size_t Length)
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

void ACS::ProcessParallel(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset, size_t Length)
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
		IntegerTools::LeIncrease8(m_acsState->Nonce, thdc, static_cast<uint>(CTRLEN * i));
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
	MemoryTools::Copy(tmpc, 0, m_acsState->Nonce, 0, BLOCK_SIZE);

	// last block processing
	const size_t ALNLEN = CNKLEN * m_parallelProfile.ParallelMaxDegree();
	if (ALNLEN < OUTLEN)
	{
		const size_t FNLLEN = OUTLEN - ALNLEN;
		InOffset += ALNLEN;
		OutOffset += ALNLEN;

		Generate(Output, OutOffset, FNLLEN, m_acsState->Nonce);

		for (size_t i = 0; i < FNLLEN; ++i)
		{
			Output[OutOffset + i] ^= Input[InOffset + i];
		}
	}
}

void ACS::ProcessSequential(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset, size_t Length)
{
	// get block aligned
	const size_t ALNLEN = Length - (Length % BLOCK_SIZE);
	size_t i;

	// generate random
	Generate(Output, OutOffset, Length, m_acsState->Nonce);

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

void ACS::Reset()
{
	m_acsState->Reset();

	if (IsAuthenticator())
	{
		m_macAuthenticator->Reset();
	}

	m_parallelProfile.Calculate(m_parallelProfile.IsParallel(), m_parallelProfile.ParallelBlockSize(), m_parallelProfile.ParallelMaxDegree());
}

SecureVector<byte> ACS::Serialize()
{
	SecureVector<byte> tmps = m_acsState->Serialize();

	return tmps;
}

void ACS::Transform256(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
{
	const size_t HLFBLK = 16;
	const size_t RNDCNT = m_acsState->RoundKeys.size() - 3;
	size_t kctr;

	__m128i blk1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&Input[InOffset]));
	__m128i blk2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&Input[InOffset + HLFBLK]));
	__m128i tmp1;
	__m128i tmp2;

	kctr = 0;
	blk1 = _mm_xor_si128(blk1, m_acsState->RoundKeys[kctr]);
	++kctr;
	blk2 = _mm_xor_si128(blk2, m_acsState->RoundKeys[kctr]);

	while (kctr != RNDCNT)
	{
		// mix the blocks
		tmp1 = _mm_blendv_epi8(blk1, blk2, BLEND_MASK);
		tmp2 = _mm_blendv_epi8(blk2, blk1, BLEND_MASK);
		// shuffle
		tmp1 = _mm_shuffle_epi8(tmp1, SHIFT_MASK);
		tmp2 = _mm_shuffle_epi8(tmp2, SHIFT_MASK);
		++kctr;
		// encrypt the first half-block
		blk1 = _mm_aesenc_si128(tmp1, m_acsState->RoundKeys[kctr]);
		++kctr;
		// encrypt the second half-block
		blk2 = _mm_aesenc_si128(tmp2, m_acsState->RoundKeys[kctr]);
	}

	// final block
	tmp1 = _mm_blendv_epi8(blk1, blk2, BLEND_MASK);
	tmp2 = _mm_blendv_epi8(blk2, blk1, BLEND_MASK);
	tmp1 = _mm_shuffle_epi8(tmp1, SHIFT_MASK);
	tmp2 = _mm_shuffle_epi8(tmp2, SHIFT_MASK);
	++kctr;
	blk1 = _mm_aesenclast_si128(tmp1, m_acsState->RoundKeys[kctr]);
	++kctr;
	blk2 = _mm_aesenclast_si128(tmp2, m_acsState->RoundKeys[kctr]);

	// store in output
	_mm_storeu_si128(reinterpret_cast<__m128i*>(&Output[OutOffset]), blk1);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(&Output[OutOffset + HLFBLK]), blk2);
}

void ACS::Transform1024(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
{
	Transform256(Input, InOffset, Output, OutOffset);
	Transform256(Input, InOffset + 32, Output, OutOffset + 32);
	Transform256(Input, InOffset + 64, Output, OutOffset + 64);
	Transform256(Input, InOffset + 96, Output, OutOffset + 96);
}

void ACS::Transform2048(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
{
	Transform1024(Input, InOffset, Output, OutOffset);
	Transform1024(Input, InOffset + 128, Output, OutOffset + 128);
}

void ACS::Transform4096(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
{
	Transform2048(Input, InOffset, Output, OutOffset);
	Transform2048(Input, InOffset + 256, Output, OutOffset + 256);
}

NAMESPACE_STREAMEND
