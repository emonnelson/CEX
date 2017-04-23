#include "CBC.h"
#include "BlockCipherFromName.h"
#include "IntUtils.h"
#include "MemUtils.h"
#include "ParallelUtils.h"

NAMESPACE_MODE

const std::string CBC::CLASS_NAME("CBC");

//~~~Properties~~~//

const size_t CBC::BlockSize()
{
	return BLOCK_SIZE;
}

const BlockCiphers CBC::CipherType()
{
	return m_cipherType;
}

IBlockCipher* CBC::Engine()
{
	return m_blockCipher;
}

const CipherModes CBC::Enumeral()
{
	return CipherModes::CBC;
}

const bool CBC::IsEncryption()
{
	return m_isEncryption;
}

const bool CBC::IsInitialized()
{
	return m_isInitialized;
}

const bool CBC::IsParallel()
{
	return m_parallelProfile.IsParallel();
}

const std::vector<SymmetricKeySize> &CBC::LegalKeySizes()
{
	return m_blockCipher->LegalKeySizes();
}

const std::string &CBC::Name()
{
	return CLASS_NAME;
}

const size_t CBC::ParallelBlockSize()
{
	return m_parallelProfile.ParallelBlockSize();
}

ParallelOptions &CBC::ParallelProfile()
{
	return m_parallelProfile;
}

//~~~Constructor~~~//

CBC::CBC(BlockCiphers CipherType)
	:
	m_blockCipher(Helper::BlockCipherFromName::GetInstance(CipherType)),
	m_cbcVector(BLOCK_SIZE),
	m_cipherType(CipherType),
	m_destroyEngine(true),
	m_isDestroyed(false),
	m_isEncryption(false),
	m_isInitialized(false),
	m_isLoaded(false),
	m_parallelProfile(BLOCK_SIZE, true, m_blockCipher->StateCacheSize(), true)
{
}

CBC::CBC(IBlockCipher* Cipher)
	:
	m_blockCipher(Cipher != 0 ? Cipher : throw CryptoCipherModeException("CBC:CTor", "The Cipher can not be null!")),
	m_cbcVector(BLOCK_SIZE),
	m_cipherType(Cipher->Enumeral()),
	m_destroyEngine(false),
	m_isDestroyed(false),
	m_isEncryption(false),
	m_isInitialized(false),
	m_isLoaded(false),
	m_parallelProfile(BLOCK_SIZE, true, m_blockCipher->StateCacheSize(), true)
{
}

CBC::~CBC()
{
	Destroy();
}

//~~~Public Functions

void CBC::DecryptBlock(const std::vector<byte> &Input, std::vector<byte> &Output)
{
	Decrypt128(Input, 0, Output, 0);
}

void CBC::DecryptBlock(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	Decrypt128(Input, InOffset, Output, OutOffset);
}

void CBC::Destroy()
{
	if (!m_isDestroyed)
	{
		m_isDestroyed = true;
		m_cipherType = BlockCiphers::None;
		m_isEncryption = false;
		m_isInitialized = false;
		m_isLoaded = false;
		m_parallelProfile.Reset();

		try
		{
			if (m_destroyEngine)
			{
				m_destroyEngine = false;

				if (m_blockCipher != 0)
					delete m_blockCipher;
			}

			Utility::IntUtils::ClearVector(m_cbcVector);
		}
		catch(std::exception& ex) 
		{
			throw CryptoCipherModeException("CBC:Destroy", "Could not clear all variables!", std::string(ex.what()));
		}
	}
}

void CBC::EncryptBlock(const std::vector<byte> &Input, std::vector<byte> &Output)
{
	Encrypt128(Input, 0, Output, 0);
}

void CBC::EncryptBlock(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	Encrypt128(Input, InOffset, Output, OutOffset);
}

void CBC::Initialize(bool Encryption, ISymmetricKey &KeyParams)
{
	if (KeyParams.Nonce().size() < 16)
		throw CryptoSymmetricCipherException("CBC:Initialize", "Requires a minimum 16 bytes of Nonce!");
	if (!SymmetricKeySize::Contains(LegalKeySizes(), KeyParams.Key().size()))
		throw CryptoSymmetricCipherException("CBC:Initialize", "Invalid key size! Key must be one of the LegalKeySizes() in length.");
	if (m_parallelProfile.IsParallel() && m_parallelProfile.ParallelBlockSize() < m_parallelProfile.ParallelMinimumSize() || m_parallelProfile.ParallelBlockSize() > m_parallelProfile.ParallelMaximumSize())
		throw CryptoSymmetricCipherException("CBC:Initialize", "The parallel block size is out of bounds!");
	if (m_parallelProfile.IsParallel() && m_parallelProfile.ParallelBlockSize() % m_parallelProfile.ParallelMinimumSize() != 0)
		throw CryptoSymmetricCipherException("CBC:Initialize", "The parallel block size must be evenly aligned to the ParallelMinimumSize!");

	Scope();
	m_blockCipher->Initialize(Encryption, KeyParams);
	m_cbcVector = KeyParams.Nonce();
	m_isEncryption = Encryption;
	m_isInitialized = true;
}

void CBC::ParallelMaxDegree(size_t Degree)
{
	if (Degree == 0)
		throw CryptoCipherModeException("CBC:ParallelMaxDegree", "Parallel degree can not be zero!");
	if (Degree % 2 != 0)
		throw CryptoCipherModeException("CBC:ParallelMaxDegree", "Parallel degree must be an even number!");
	if (Degree > m_parallelProfile.ProcessorCount())
		throw CryptoCipherModeException("CBC:ParallelMaxDegree", "Parallel degree can not exceed processor count!");

	m_parallelProfile.SetMaxDegree(Degree);
}

void CBC::Transform(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset, const size_t Length)
{
	Process(Input, InOffset, Output, OutOffset, Length);
}

//~~~Private Functions~~~//

void CBC::Decrypt128(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	CEXASSERT(m_isInitialized, "The cipher mode has not been initialized!");
	CEXASSERT(Utility::IntUtils::Min(Input.size() - InOffset, Output.size() - OutOffset) >= BLOCK_SIZE, "The data arrays are smaller than the the block-size!");

	std::vector<byte> nxtIv(BLOCK_SIZE);
	Utility::MemUtils::COPY128<byte, byte>(Input, InOffset, nxtIv, 0);
	m_blockCipher->DecryptBlock(Input, InOffset, Output, OutOffset);
	Utility::MemUtils::XOR128<byte>(m_cbcVector, 0, Output, OutOffset);
	Utility::MemUtils::COPY128<byte, byte>(nxtIv, 0, m_cbcVector, 0);
}

void CBC::DecryptParallel(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	const size_t SEGSZE = m_parallelProfile.ParallelBlockSize() / m_parallelProfile.ParallelMaxDegree();
	const size_t BLKCNT = (SEGSZE / BLOCK_SIZE);
	std::vector<byte> tmpIv(BLOCK_SIZE);

	Utility::ParallelUtils::ParallelFor(0, m_parallelProfile.ParallelMaxDegree(), [this, &Input, InOffset, &Output, OutOffset, &tmpIv, SEGSZE, BLKCNT](size_t i)
	{
		std::vector<byte> thdIv(BLOCK_SIZE);

		if (i != 0)
			Utility::MemUtils::COPY128<byte, byte>(Input, (InOffset + (i * SEGSZE)) - BLOCK_SIZE, thdIv, 0);
		else
			Utility::MemUtils::COPY128<byte, byte>(m_cbcVector, 0, thdIv, 0);

		this->DecryptSegment(Input, InOffset + i * SEGSZE, Output, OutOffset + i * SEGSZE, thdIv, BLKCNT);

		if (i == m_parallelProfile.ParallelMaxDegree() - 1)
			Utility::MemUtils::COPY128<byte, byte>(thdIv, 0, tmpIv, 0);
	});

	Utility::MemUtils::COPY128<byte, byte>(tmpIv, 0, m_cbcVector, 0);
}

void CBC::DecryptSegment(const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset, std::vector<byte> &Iv, const size_t BlockCount)
{
	size_t blkCtr = BlockCount;

#if defined(__AVX512__)
	if (blkCtr > 15)
	{
		// 512bit avx
		const size_t AVX512BLK = 256;
		size_t rndCtr = (blkCtr / 16);
		std::vector<byte> blkIv(AVX512BLK);
		std::vector<byte> blkNxt(AVX512BLK);
		const size_t BLKOFT = AVX512BLK - Iv.size();

		// build wide iv
		Utility::MemUtils::COPY128<byte, byte>(Iv, 0, blkIv, 0);
		Utility::MemUtils::Copy<byte>(Input, InOffset, blkIv, BLOCK_SIZE, BLKOFT);

		while (rndCtr != 0)
		{
			const size_t INPOFT = InOffset + BLKOFT;
			// store next iv
			Utility::MemUtils::Copy<byte>(Input, INPOFT, blkNxt, 0, (Input.size() - INPOFT >= AVX512BLK) ? AVX512BLK : Input.size() - INPOFT);
			// transform 8 blocks
			m_blockCipher->Transform2048(Input, InOffset, Output, OutOffset);
			// xor the set
			Utility::MemUtils::XOR1024<byte>(blkIv, 0, Output, OutOffset);
			Utility::MemUtils::XOR1024<byte>(blkIv + 128, 0, Output, OutOffset + 128);
			// swap iv
			Utility::MemUtils::Copy<byte>(blkNxt, 0, blkIv, 0, AVX512BLK);
			InOffset += AVX512BLK;
			OutOffset += AVX512BLK;
			blkCtr -= 16;
			--rndCtr;
		}

		Utility::MemUtils::COPY128<byte, byte>(blkNxt, 0, Iv, 0);
	}
#elif defined(__AVX2__)
	if (blkCtr > 7)
	{
		// 256bit avx
		const size_t AVX2BLK = 128;
		size_t rndCtr = (blkCtr / 8);
		std::vector<byte> blkIv(AVX2BLK);
		std::vector<byte> blkNxt(AVX2BLK);
		const size_t BLKOFT = AVX2BLK - Iv.size();

		// build wide iv
		Utility::MemUtils::COPY128<byte, byte>(Iv, 0, blkIv, 0);
		Utility::MemUtils::Copy<byte>(Input, InOffset, blkIv, BLOCK_SIZE, BLKOFT);

		while (rndCtr != 0)
		{
			const size_t INPOFT = InOffset + BLKOFT;
			// store next iv
			Utility::MemUtils::Copy<byte>(Input, INPOFT, blkNxt, 0, (Input.size() - INPOFT >= AVX2BLK) ? AVX2BLK: Input.size() - INPOFT);
			// transform 8 blocks
			m_blockCipher->Transform1024(Input, InOffset, Output, OutOffset);
			// xor the set
			Utility::MemUtils::XOR1024<byte>(blkIv, 0, Output, OutOffset);
			// swap iv
			Utility::MemUtils::Copy<byte>(blkNxt, 0, blkIv, 0, AVX2BLK);
			InOffset += AVX2BLK;
			OutOffset += AVX2BLK;
			blkCtr -= 8;
			--rndCtr;
		}

		Utility::MemUtils::COPY128<byte, byte>(blkNxt, 0, Iv, 0);
	}
#elif defined(__AVX__)
	if (blkCtr > 3)
	{
		// 128bit sse3
		const size_t AVXBLK = 64;
		size_t rndCtr = (blkCtr / 4);
		std::vector<byte> blkIv(AVXBLK);
		std::vector<byte> blkNxt(AVXBLK);
		const size_t BLKOFT = AVXBLK - Iv.size();

		Utility::MemUtils::COPY128<byte, byte>(Iv, 0, blkIv, 0);
		Utility::MemUtils::Copy<byte>(Input, InOffset, blkIv, BLOCK_SIZE, BLKOFT);

		while (rndCtr != 0)
		{
			const size_t INPOFT = InOffset + BLKOFT;
			Utility::MemUtils::Copy<byte>(Input, INPOFT, blkNxt, 0, (Input.size() - INPOFT >= AVXBLK) ? AVXBLK : Input.size() - INPOFT);
			m_blockCipher->Transform512(Input, InOffset, Output, OutOffset);
			Utility::MemUtils::XOR512<byte>(blkIv, 0, Output, OutOffset);
			Utility::MemUtils::Copy<byte>(blkNxt, 0, blkIv, 0, AVXBLK);
			InOffset += AVXBLK;
			OutOffset += AVXBLK;
			blkCtr -= 4;
			--rndCtr;
		}

		Utility::MemUtils::COPY128<byte, byte>(blkNxt, 0, Iv, 0);
	}
#endif

	if (blkCtr != 0)
	{
		// Note: if it's hitting this, your parallel block size is misaligned
		std::vector<byte> nxtIv(BLOCK_SIZE);

		while (blkCtr != 0)
		{
			Utility::MemUtils::COPY128<byte, byte>(Input, InOffset, nxtIv, 0);
			m_blockCipher->DecryptBlock(Input, InOffset, Output, OutOffset);
			Utility::MemUtils::XOR128<byte>(Iv, 0, Output, OutOffset);
			Utility::MemUtils::COPY128<byte, byte>(nxtIv, 0, Iv, 0);
			InOffset += BLOCK_SIZE;
			OutOffset += BLOCK_SIZE;
			--blkCtr;
		}
	}
}

void CBC::Encrypt128(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset)
{
	CEXASSERT(m_isInitialized, "The cipher mode has not been initialized!");
	CEXASSERT(Utility::IntUtils::Min(Input.size() - InOffset, Output.size() - OutOffset) >= BLOCK_SIZE, "The data arrays are smaller than the the block-size!");

	Utility::MemUtils::XOR128<byte>(Input, InOffset, m_cbcVector, 0);
	m_blockCipher->EncryptBlock(m_cbcVector, 0, Output, OutOffset);
	Utility::MemUtils::COPY128<byte, byte>(Output, OutOffset, m_cbcVector, 0);
}

void CBC::Process(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset, const size_t Length)
{
	CEXASSERT(m_isInitialized, "The cipher mode has not been initialized!");
	CEXASSERT(Utility::IntUtils::Min(Input.size() - InOffset, Output.size() - OutOffset) >= Length, "The data arrays are smaller than the the block-size!");
	CEXASSERT(Length % m_blockCipher->BlockSize() == 0, "The length must be evenly divisible by the block ciphers block-size!");

	size_t blkCtr = Length / BLOCK_SIZE;

	if (m_isEncryption)
	{
		for (size_t i = 0; i < blkCtr; ++i)
			Encrypt128(Input, (i * BLOCK_SIZE) + InOffset, Output, (i * BLOCK_SIZE) + OutOffset);
	}
	else
	{
		if (m_parallelProfile.IsParallel() && Length >= m_parallelProfile.ParallelBlockSize())
		{
			const size_t PRBCNT = Length / m_parallelProfile.ParallelBlockSize();

			for (size_t i = 0; i < PRBCNT; ++i)
				DecryptParallel(Input, (i * m_parallelProfile.ParallelBlockSize()) + InOffset, Output, (i * m_parallelProfile.ParallelBlockSize()) + OutOffset);

			const size_t PRCBLK = (m_parallelProfile.ParallelBlockSize() / BLOCK_SIZE) * PRBCNT;
			blkCtr -= PRCBLK;

			for (size_t i = 0; i < blkCtr; ++i)
				Decrypt128(Input, ((i + PRCBLK) * BLOCK_SIZE) + InOffset, Output, ((i + PRCBLK) * BLOCK_SIZE) + OutOffset);
		}
		else
		{
			for (size_t i = 0; i < blkCtr; ++i)
				Decrypt128(Input, (i * BLOCK_SIZE) + InOffset, Output, (i * BLOCK_SIZE) + OutOffset);
		}
	}
}

void CBC::Scope()
{
	if (!m_parallelProfile.IsDefault())
		m_parallelProfile.Calculate();
}

NAMESPACE_MODEEND