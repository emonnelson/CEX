#include "CipherStreamTest.h"
#include "../CEX/BlockCipherExtensions.h"
#include "../CEX/CipherStream.h"
#include "../CEX/FileStream.h"
#include "../CEX/MemoryStream.h"
#include "../CEX/SecureRandom.h"
#include "../CEX/CTR.h"
#include "../CEX/CBC.h"
#include "../CEX/CFB.h"
#include "../CEX/OFB.h"
#include "../CEX/X923.h"
#include "../CEX/PKCS7.h"
#include "../CEX/TBC.h"
#include "../CEX/ISO7816.h"
#include "../CEX/ParallelUtils.h"
#include "../CEX/RHX.h"
#include "../CEX/SHX.h"
#include "../CEX/ChaCha256.h"

namespace Test
{
	const std::string CipherStreamTest::DESCRIPTION = "CipherStream Processer Tests.";
	const std::string CipherStreamTest::FAILURE = "FAILURE: ";
	const std::string CipherStreamTest::SUCCESS = "SUCCESS! CipherStream tests have executed succesfully.";

	CipherStreamTest::CipherStreamTest()
		:
		m_cmpText(MAX_ALLOC),
		m_decText(MAX_ALLOC),
		m_encText(MAX_ALLOC),
		m_iv(16),
		m_key(32),
		m_plnText(MAX_ALLOC),
		m_processorCount(1),
		m_progressEvent()
	{
	}

	CipherStreamTest::~CipherStreamTest()
	{
	}

	const std::string CipherStreamTest::Description()
	{
		return DESCRIPTION;
	}

	TestEventHandler &CipherStreamTest::Progress()
	{
		return m_progressEvent;
	}

	std::string CipherStreamTest::Run()
	{
		using namespace Cipher::Symmetric::Block::Mode;
		using namespace Cipher::Symmetric::Block::Padding;
		using namespace Enumeration;

		try
		{
			Initialize();

			// local test
			//FileStreamTest();

			CbcModeTest();
			OnProgress(std::string("Passed CBC Mode tests.."));
			CfbModeTest();
			OnProgress(std::string("Passed CFB Mode tests.."));
			CtrModeTest();
			OnProgress(std::string("Passed CTR Mode tests.."));
			OfbModeTest();
			OnProgress(std::string("Passed OFB Mode tests.."));

			MemoryStreamTest();
			OnProgress(std::string("Passed MemoryStream self test.. "));

			SerializeStructTest();
			OnProgress(std::string("Passed CipherDescription serialization test.."));

			OnProgress(std::string("***Testing Cipher Parameters***"));
			ParametersTest();
			OnProgress(std::string("Passed Cipher Parameters tests.."));

			Cipher::Symmetric::Block::RHX* eng = new Cipher::Symmetric::Block::RHX();
			OnProgress(std::string("***Testing Padding Modes***"));
			StreamModesTest(new CBC(eng), new X923());
			OnProgress(std::string("Passed CBC/X923 CipherStream test.."));
			StreamModesTest(new CBC(eng), new PKCS7());
			OnProgress(std::string("Passed CBC/PKCS7 CipherStream test.."));
			StreamModesTest(new CBC(eng), new TBC());
			OnProgress(std::string("Passed CBC/TBC CipherStream test.."));
			StreamModesTest(new CBC(eng), new ISO7816());
			OnProgress(std::string("Passed CBC/ISO7816 CipherStream test.."));

			OnProgress(std::string("***Testing Cipher Modes***"));
			StreamModesTest(new CTR(eng), new ISO7816());
			OnProgress(std::string("Passed CTR CipherStream test.."));
			StreamModesTest(new CFB(eng), new ISO7816());
			OnProgress(std::string("Passed CFB CipherStream test.."));
			StreamModesTest(new OFB(eng), new ISO7816());
			OnProgress(std::string("Passed OFB CipherStream test.."));
			delete eng;

			OnProgress(std::string("***Testing Cipher Description Initialization***"));
			//Processing::CipherDescription cd2(BlockCiphers::Rijndael,)
			Processing::CipherDescription cd(
				Enumeration::BlockCiphers::Rijndael,		// cipher engine
				Enumeration::BlockCipherExtensions::None,	// cipher extension
				Enumeration::CipherModes::CTR,				// cipher mode
				Enumeration::PaddingModes::None,			// padding mode
				Enumeration::KeySizes::K256,				// key size
				Enumeration::IVSizes::V128);				// cipher iv size

			DescriptionTest(&cd);
			OnProgress(std::string("Passed CipherDescription stream test.."));
			Cipher::Symmetric::Block::SHX* spx = new Cipher::Symmetric::Block::SHX();
			StreamModesTest(new CBC(spx), new ISO7816());
			delete spx;
			OnProgress(std::string("Passed SHX CipherStream test.."));

			m_key.resize(192);
			for (size_t i = 0; i < 192; i++)
			{
				m_key[i] = (byte)i;
			}

			// test extended ciphers
			Cipher::Symmetric::Block::RHX* rhx = new Cipher::Symmetric::Block::RHX();
			StreamModesTest(new CBC(rhx), new ISO7816());
			delete rhx;
			OnProgress(std::string("Passed RHX extended CipherStream test.."));
			Cipher::Symmetric::Block::SHX* shx = new Cipher::Symmetric::Block::SHX();
			StreamModesTest(new CBC(shx), new ISO7816());
			delete shx;

			return SUCCESS;
		}
		catch (TestException const &ex)
		{
			throw TestException(FAILURE + std::string(" : ") + ex.Message());
		}
		catch (...)
		{
			throw TestException(std::string(FAILURE + std::string(" : Unknown Error")));
		}
	}

	void CipherStreamTest::FileStreamTest()
	{
		using namespace CEX::IO;

		const std::string INPFILE = "C:\\Users\\John\\Documents\\Tests\\test1.txt"; // input
		const std::string ENCFILE = "C:\\Users\\John\\Documents\\Tests\\test2.txt"; // empty
		const std::string DECFILE = "C:\\Users\\John\\Documents\\Tests\\test3.txt"; // empty
		std::vector<byte> key(32, 1);
		std::vector<byte> iv(16, 2);
		std::vector<byte> data(1025, 3);

		// initialize the cipher and key container
		Processing::CipherStream cs(Enumeration::BlockCiphers::Rijndael, Enumeration::BlockCipherExtensions::None, Enumeration::CipherModes::CBC, Enumeration::PaddingModes::ISO7816);
		Key::Symmetric::SymmetricKey kp(key, iv);

		// encrypt the file in-place
		FileStream fIn1(INPFILE, FileStream::FileAccess::Read);
		FileStream fOut1(INPFILE, FileStream::FileAccess::ReadWrite);
		cs.Initialize(true, kp);
		cs.Write(&fIn1, &fOut1);
		fIn1.Close();
		fOut1.Close();

		// decrypt the file in-place
		FileStream fIn2(INPFILE, FileStream::FileAccess::Read);
		FileStream fOut2(INPFILE, FileStream::FileAccess::ReadWrite);
		cs.Initialize(false, kp);
		cs.Write(&fIn2, &fOut2);
		fIn2.Close();
		fOut2.Close();

		// encrypt and copy to a new file
		FileStream fIn3(INPFILE, FileStream::FileAccess::Read);
		FileStream fOut3(ENCFILE, FileStream::FileAccess::ReadWrite);
		cs.Initialize(true, kp);
		cs.Write(&fIn3, &fOut3);
		fIn3.Close();
		fOut3.Close();

		// decrypt and copy to a new file
		FileStream fIn4(ENCFILE, FileStream::FileAccess::Read);
		FileStream fOut4(DECFILE, FileStream::FileAccess::ReadWrite);
		cs.Initialize(false, kp);
		cs.Write(&fIn4, &fOut4);
		fIn4.Close();
		fOut4.Close();/**/
	}

	void CipherStreamTest::CbcModeTest()
	{
		AllocateRandom(m_iv, 16);
		AllocateRandom(m_key, 32);

		Key::Symmetric::SymmetricKey kp(m_key, m_iv);
		Cipher::Symmetric::Block::RHX* eng = new Cipher::Symmetric::Block::RHX();
		Cipher::Symmetric::Block::Mode::CBC cipher(eng);
		Cipher::Symmetric::Block::Mode::CBC cipher2(eng);
		Cipher::Symmetric::Block::Padding::ISO7816* padding = new Cipher::Symmetric::Block::Padding::ISO7816();
		cipher.ParallelProfile().IsParallel() = false;
		Processing::CipherStream cs(&cipher2, padding);
		Prng::SecureRandom rng;

		for (size_t i = 0; i < 10; i++)
		{
			uint smpSze = rng.NextUInt32(static_cast<uint>(cipher.ParallelProfile().ParallelMinimumSize() * 4), static_cast<uint>(cipher.ParallelProfile().ParallelMinimumSize()));
			size_t prlBlock = smpSze - (smpSze % cipher.ParallelProfile().ParallelMinimumSize());
			AllocateRandom(m_plnText, smpSze);
			m_cmpText.resize(smpSze);
			m_decText.resize(smpSze);
			m_encText.resize(smpSze);

			cipher.ParallelProfile().ParallelBlockSize() = prlBlock;
			cipher2.ParallelProfile().ParallelBlockSize() = prlBlock;
			IO::MemoryStream mIn(m_plnText);
			IO::MemoryStream mOut;
			IO::MemoryStream mRes;

			// *** Compare encryption output *** //

			// local processor
			cipher.Initialize(true, kp);
			BlockEncrypt(&cipher, padding, m_plnText, 0, m_encText, 0);

			// streamcipher linear mode
			cs.ParallelProfile().IsParallel() = false;
			// memorystream interface
			cs.Initialize(true, kp);
			cs.Write(&mIn, &mOut);

			if (mOut.ToArray() != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			// byte array interface
			cs.Initialize(true, kp);
			cs.Write(m_plnText, 0, m_cmpText, 0);

			if (m_cmpText != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			// ***compare decryption output *** //

			// local processor
			cipher.Initialize(false, kp);
			BlockDecrypt(&cipher, padding, m_encText, 0, m_decText, 0);

			if (m_plnText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// decrypt linear mode
			cs.ParallelProfile().IsParallel() = false;
			mOut.Seek(0, IO::SeekOrigin::Begin);
			cs.Initialize(false, kp);
			cs.Write(&mOut, &mRes);

			if (mRes.ToArray() != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// byte array interface
			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_cmpText, 0);

			if (m_cmpText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// decrypt parallel mode
			cs.ParallelProfile().IsParallel() = true;
			cs.ParallelProfile().ParallelBlockSize() = prlBlock;
			mOut.Seek(0, IO::SeekOrigin::Begin);
			mRes.Seek(0, IO::SeekOrigin::Begin);
			cs.Initialize(false, kp);
			cs.Write(&mOut, &mRes);

			if (mRes.ToArray() != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			m_cmpText.resize(m_encText.size());
			// byte array interface parallel
			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_cmpText, 0);

			if (m_cmpText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			m_cmpText.resize(m_encText.size());
			// byte array interface sequential
			cs.ParallelProfile().IsParallel() = false;
			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_cmpText, 0);

			if (m_cmpText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}
		}

		delete eng;
		delete padding;
	}

	void CipherStreamTest::CfbModeTest()
	{
		AllocateRandom(m_iv, 16);
		AllocateRandom(m_key, 32);
		Key::Symmetric::SymmetricKey kp(m_key, m_iv);
		Cipher::Symmetric::Block::RHX* eng = new Cipher::Symmetric::Block::RHX();
		Cipher::Symmetric::Block::Mode::CFB cipher(eng);
		Cipher::Symmetric::Block::Mode::CFB cipher2(eng);
		Cipher::Symmetric::Block::Padding::ISO7816* padding = new Cipher::Symmetric::Block::Padding::ISO7816();
		cipher.ParallelProfile().IsParallel() = false;
		Processing::CipherStream cs(&cipher2, padding);
		Prng::SecureRandom rng;

		for (size_t i = 0; i < 10; i++)
		{
			uint smpSze = rng.NextUInt32(static_cast<uint>(cipher.ParallelProfile().ParallelMinimumSize() * 4), static_cast<uint>(cipher.ParallelProfile().ParallelMinimumSize()));
			size_t prlBlock = smpSze - (smpSze % cipher.ParallelProfile().ParallelMinimumSize());
			AllocateRandom(m_plnText, smpSze);
			m_cmpText.resize(smpSze);
			m_decText.resize(smpSze);
			m_encText.resize(smpSze);

			cipher.ParallelProfile().ParallelBlockSize() = prlBlock;
			cipher2.ParallelProfile().ParallelBlockSize() = prlBlock;
			IO::MemoryStream mIn(m_plnText);
			IO::MemoryStream mOut;
			IO::MemoryStream mRes;

			// *** Compare encryption output *** //

			// local processor
			cipher.Initialize(true, kp);
			BlockEncrypt(&cipher, padding, m_plnText, 0, m_encText, 0);

			// streamcipher linear mode
			cs.ParallelProfile().IsParallel() = false;
			// memorystream interface
			cs.Initialize(true, kp);
			cs.Write(&mIn, &mOut);

			if (mOut.ToArray() != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			// byte array interface
			cs.Initialize(true, kp);
			cs.Write(m_plnText, 0, m_cmpText, 0);

			if (m_cmpText != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			// ***compare decryption output *** //

			// local processor
			cipher.Initialize(false, kp);
			BlockDecrypt(&cipher, padding, m_encText, 0, m_decText, 0);

			if (m_plnText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// decrypt linear mode
			cs.ParallelProfile().IsParallel() = false;
			mOut.Seek(0, IO::SeekOrigin::Begin);
			cs.Initialize(false, kp);
			cs.Write(&mOut, &mRes);

			if (mRes.ToArray() != m_plnText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// byte array interface
			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_cmpText, 0);

			if (m_cmpText != m_plnText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// decrypt parallel mode
			cs.ParallelProfile().IsParallel() = true;
			cs.ParallelProfile().ParallelBlockSize() = prlBlock;
			mOut.Seek(0, IO::SeekOrigin::Begin);
			mRes.Seek(0, IO::SeekOrigin::Begin);
			cs.Initialize(false, kp);
			cs.Write(&mOut, &mRes);

			if (mRes.ToArray() != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			m_cmpText.resize(m_encText.size());
			// byte array interface parallel
			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_cmpText, 0);

			if (m_cmpText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			m_cmpText.resize(m_encText.size());
			// byte array interface sequential
			cs.ParallelProfile().IsParallel() = false;
			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_cmpText, 0);

			if (m_cmpText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}
		}

		delete eng;
		delete padding;
	}

	void CipherStreamTest::CtrModeTest()
	{
		AllocateRandom(m_iv, 16);
		AllocateRandom(m_key, 32);

		Key::Symmetric::SymmetricKey kp(m_key, m_iv);
		Cipher::Symmetric::Block::RHX* eng = new Cipher::Symmetric::Block::RHX();
		Cipher::Symmetric::Block::Mode::CTR cipher(eng);
		Cipher::Symmetric::Block::Mode::CTR cipher2(eng);
		Processing::CipherStream cs(&cipher2);
		cipher.ParallelProfile().IsParallel() = false;
		Prng::SecureRandom rng;

		// ctr test
		for (size_t i = 0; i < 10; i++)
		{
			uint smpSze = rng.NextUInt32(static_cast<uint>(cipher.ParallelProfile().ParallelMinimumSize() * 4), static_cast<uint>(cipher.ParallelProfile().ParallelMinimumSize()));
			size_t prlBlock = smpSze - (smpSze % cipher.ParallelProfile().ParallelMinimumSize());
			AllocateRandom(m_plnText, smpSze);
			m_encText.resize(smpSze);
			m_cmpText.resize(smpSze);
			m_decText.resize(smpSze);

			cipher.ParallelProfile().ParallelBlockSize() = prlBlock;
			cipher2.ParallelProfile().ParallelBlockSize() = prlBlock;
			IO::MemoryStream mIn(m_plnText);
			IO::MemoryStream mOut;
			IO::MemoryStream mRes;

			// *** Compare encryption output *** //

			// local processor
			cipher.Initialize(true, kp);
			BlockCTR(&cipher, m_plnText, 0, m_encText, 0);

			// streamcipher linear mode
			cs.ParallelProfile().IsParallel() = false;
			// memorystream interface
			cs.Initialize(true, kp);
			cs.Write(&mIn, &mOut);

			if (mOut.ToArray() != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			// byte array interface
			cs.Initialize(true, kp);
			cs.Write(m_plnText, 0, m_cmpText, 0);

			if (m_cmpText != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			mIn.Seek(0, IO::SeekOrigin::Begin);
			mOut.Seek(0, IO::SeekOrigin::Begin);

			cs.ParallelProfile().IsParallel() = true;
			cs.ParallelProfile().ParallelBlockSize() = prlBlock;
			cs.Initialize(true, kp);
			cs.Write(&mIn, &mOut);

			if (mOut.ToArray() != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			// byte array interface
			cs.Initialize(true, kp);
			cs.Write(m_plnText, 0, m_cmpText, 0);

			if (m_cmpText != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			// ***compare decryption output *** //

			// local processor
			cipher.Initialize(false, kp);
			BlockCTR(&cipher, m_encText, 0, m_decText, 0);

			if (m_plnText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// decrypt linear mode
			cs.ParallelProfile().IsParallel() = false;
			mOut.Seek(0, IO::SeekOrigin::Begin);
			cs.Initialize(false, kp);
			cs.Write(&mOut, &mRes);

			if (mRes.ToArray() != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// byte array interface
			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_cmpText, 0);

			if (m_cmpText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// decrypt parallel mode
			cs.ParallelProfile().IsParallel() = true;
			cs.ParallelProfile().ParallelBlockSize() = prlBlock;
			mOut.Seek(0, IO::SeekOrigin::Begin);
			mRes.Seek(0, IO::SeekOrigin::Begin);
			cs.Initialize(false, kp);
			cs.Write(&mOut, &mRes);

			if (mRes.ToArray() != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// byte array interface
			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_cmpText, 0);

			if (m_cmpText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}
		}

		delete eng;
	}

	void CipherStreamTest::DescriptionTest(Processing::CipherDescription* Description)
	{
		AllocateRandom(m_iv, 16);
		AllocateRandom(m_key, 32);
		AllocateRandom(m_plnText);

		Key::Symmetric::SymmetricKey kp(m_key, m_iv);
		IO::MemoryStream mIn(m_plnText);
		IO::MemoryStream mOut;
		IO::MemoryStream mRes;

		Processing::CipherStream cs(Description);
		cs.Initialize(true, kp);
		cs.Write(&mIn, &mOut);

		mOut.Seek(0, IO::SeekOrigin::Begin);

		cs.Initialize(false, kp);
		cs.Write(&mOut, &mRes);

		if (mRes.ToArray() != m_plnText)
		{
			throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
		}
	}

	void CipherStreamTest::Initialize()
	{
		m_encText.reserve(MAX_ALLOC);
		m_cmpText.reserve(MAX_ALLOC);
		m_decText.reserve(MAX_ALLOC);
		m_plnText.reserve(MAX_ALLOC);
		m_processorCount = Utility::ParallelUtils::ProcessorCount();
	}

	void CipherStreamTest::MemoryStreamTest()
	{
		IO::MemoryStream ms;
		ms.WriteByte((byte)10);
		ms.WriteByte((byte)11);
		ms.WriteByte((byte)12);

		std::vector<byte> data(255);
		for (size_t i = 0; i < 255; i++)
		{
			data[i] = (byte)i;
		}
		ms.Write(data, 0, 255);

		ms.Seek(0, IO::SeekOrigin::Begin);

		byte x = ms.ReadByte();
		if (x != (byte)10)
		{
			throw;
		}
		x = ms.ReadByte();
		if (x != (byte)11)
		{
			throw;
		}
		x = ms.ReadByte();
		if (x != (byte)12)
		{
			throw;
		}
		std::vector<byte> data2(255);
		ms.Read(data2, 0, 255);
		if (data2 != data)
		{
			throw;
		}
	}

	void CipherStreamTest::ParametersTest()
	{
		AllocateRandom(m_iv, 16);
		AllocateRandom(m_key, 32);
		AllocateRandom(m_plnText, 1);
		Prng::SecureRandom rng;
		Key::Symmetric::SymmetricKey kp(m_key, m_iv);
		m_cmpText.resize(1);
		m_decText.resize(1);
		m_encText.resize(1);

		Cipher::Symmetric::Block::RHX* engine = new Cipher::Symmetric::Block::RHX();

		// 1 byte with byte arrays
		{
			Cipher::Symmetric::Block::Mode::CTR* cipher = new Cipher::Symmetric::Block::Mode::CTR(engine);
			Processing::CipherStream cs(cipher);

			cs.Initialize(true, kp);
			cs.Write(m_plnText, 0, m_encText, 0);

			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_decText, 0);

			if (m_decText != m_plnText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			delete cipher;
		}
		// 1 byte with stream
		{
			Cipher::Symmetric::Block::Mode::CTR* cipher = new Cipher::Symmetric::Block::Mode::CTR(engine);
			Processing::CipherStream cs(cipher);
			cs.Initialize(true, kp);
			AllocateRandom(m_plnText, 1);
			IO::MemoryStream mIn(m_plnText);
			IO::MemoryStream mOut;
			cs.Write(&mIn, &mOut);

			cs.Initialize(false, kp);
			IO::MemoryStream mRes;
			mOut.Seek(0, IO::SeekOrigin::Begin);
			cs.Write(&mOut, &mRes);

			if (mRes.ToArray() != m_plnText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			delete cipher;
		}

		// partial block with byte arrays
		{
			Cipher::Symmetric::Block::Mode::CTR* cipher = new Cipher::Symmetric::Block::Mode::CTR(engine);
			Processing::CipherStream cs(cipher);
			AllocateRandom(m_plnText, 15);
			m_decText.resize(15);
			m_encText.resize(15);

			cs.Initialize(true, kp);
			cs.Write(m_plnText, 0, m_encText, 0);

			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_decText, 0);

			if (m_decText != m_plnText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			delete cipher;
		}
		// partial block with stream
		{
			Cipher::Symmetric::Block::Mode::CTR* cipher = new Cipher::Symmetric::Block::Mode::CTR(engine);
			Processing::CipherStream cs(cipher);
			AllocateRandom(m_plnText, 15);
			m_decText.resize(15);
			m_encText.resize(15);

			cs.Initialize(true, kp);
			IO::MemoryStream mIn(m_plnText);
			IO::MemoryStream mOut;
			cs.Write(&mIn, &mOut);

			cs.Initialize(false, kp);
			IO::MemoryStream mRes;
			mOut.Seek(0, IO::SeekOrigin::Begin);
			cs.Write(&mOut, &mRes);

			if (mRes.ToArray() != m_plnText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			delete cipher;
		}

		// random block sizes with byte arrays
		{
			for (size_t i = 0; i < 10; i++)
			{
				Cipher::Symmetric::Block::Mode::CTR* cipher = new Cipher::Symmetric::Block::Mode::CTR(engine);
				uint smpSze = rng.NextUInt32(static_cast<uint>(cipher->ParallelProfile().ParallelMinimumSize() * 4), static_cast<uint>(cipher->ParallelProfile().ParallelMinimumSize()));
				size_t prlBlock = smpSze - (smpSze % cipher->ParallelProfile().ParallelMinimumSize());
				AllocateRandom(m_plnText, smpSze);
				m_decText.resize(smpSze);
				m_encText.resize(smpSze);

				Processing::CipherStream cs(cipher);
				cs.ParallelProfile().ParallelBlockSize() = prlBlock;
				cs.Initialize(true, kp);
				cs.Write(m_plnText, 0, m_encText, 0);

				cs.Initialize(false, kp);
				cs.Write(m_encText, 0, m_decText, 0);

				if (m_decText != m_plnText)
				{
					throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
				}

				delete cipher;
			}
		}
		// random block sizes with stream
		{
			for (size_t i = 0; i < 10; i++)
			{
				Cipher::Symmetric::Block::Mode::CTR* cipher = new Cipher::Symmetric::Block::Mode::CTR(engine);
				uint smpSze = rng.NextUInt32(static_cast<uint>(cipher->ParallelProfile().ParallelMinimumSize() * 4), static_cast<uint>(cipher->ParallelProfile().ParallelMinimumSize()));
				size_t prlBlock = smpSze - (smpSze % cipher->ParallelProfile().ParallelMinimumSize());
				AllocateRandom(m_plnText, smpSze);
				m_decText.resize(smpSze);
				m_encText.resize(smpSze);

				Processing::CipherStream cs(cipher);
				cs.ParallelProfile().ParallelBlockSize() = prlBlock;
				cs.Initialize(true, kp);
				IO::MemoryStream mIn(m_plnText);
				IO::MemoryStream mOut;
				cs.Write(&mIn, &mOut);

				cs.Initialize(false, kp);
				IO::MemoryStream mRes;
				mOut.Seek(0, IO::SeekOrigin::Begin);
				cs.Write(&mOut, &mRes);

				if (mRes.ToArray() != m_plnText)
				{
					throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
				}

				delete cipher;
			}
		}

		delete engine;
	}

	void CipherStreamTest::OfbModeTest()
	{
		AllocateRandom(m_iv, 16);
		AllocateRandom(m_key, 32);

		Key::Symmetric::SymmetricKey kp(m_key, m_iv);
		Cipher::Symmetric::Block::RHX* engine = new Cipher::Symmetric::Block::RHX();
		Cipher::Symmetric::Block::Mode::OFB cipher(engine);
		Cipher::Symmetric::Block::Mode::OFB cipher2(engine);
		Cipher::Symmetric::Block::Padding::ISO7816* padding = new Cipher::Symmetric::Block::Padding::ISO7816();
		cipher.ParallelProfile().IsParallel() = false;
		Processing::CipherStream cs(&cipher2, padding);
		Prng::SecureRandom rng;

		for (size_t i = 0; i < 10; i++)
		{
			uint smpSze = rng.NextUInt32(static_cast<uint>(cipher.ParallelProfile().ParallelMinimumSize() * 4), static_cast<uint>(cipher.ParallelProfile().ParallelMinimumSize()));
			size_t prlBlock = (size_t)smpSze - (smpSze % cipher.ParallelProfile().ParallelMinimumSize());
			AllocateRandom(m_plnText, smpSze);
			m_cmpText.resize(smpSze);
			m_decText.resize(smpSze);
			m_encText.resize(smpSze);

			cipher.ParallelProfile().ParallelBlockSize() = prlBlock;
			cipher2.ParallelProfile().ParallelBlockSize() = prlBlock;
			IO::MemoryStream mIn(m_plnText);
			IO::MemoryStream mOut;
			IO::MemoryStream mRes;

			// *** Compare encryption output *** //

			// local processor
			cipher.Initialize(true, kp);
			BlockEncrypt(&cipher, padding, m_plnText, 0, m_encText, 0);

			// streamcipher linear mode
			cs.ParallelProfile().IsParallel() = false;
			// memorystream interface
			cs.Initialize(true, kp);
			cs.Write(&mIn, &mOut);

			if (mOut.ToArray() != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			// byte array interface
			cs.Initialize(true, kp);
			cs.Write(m_plnText, 0, m_cmpText, 0);

			if (m_cmpText != m_encText)
			{
				throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
			}

			// ***compare decryption output *** //

			// local processor
			cipher.Initialize(false, kp);
			BlockDecrypt(&cipher, padding, m_encText, 0, m_decText, 0);

			if (m_plnText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			// decrypt linear mode
			cipher2.ParallelProfile().IsParallel() = false;
			mOut.Seek(0, IO::SeekOrigin::Begin);
			cs.Initialize(false, kp);
			cs.Write(&mOut, &mRes);

			if (mRes.ToArray() != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}

			m_cmpText.resize(m_encText.size());
			// byte array interface
			cs.Initialize(false, kp);
			cs.Write(m_encText, 0, m_cmpText, 0);

			if (m_cmpText != m_decText)
			{
				throw TestException(std::string("CipherStreamTest: Decrypted arrays are not equal!"));
			}
		}

		delete engine;
		delete padding;
	}

	void CipherStreamTest::SerializeStructTest()
	{
		using namespace Enumeration;

		Processing::CipherDescription cd(
			Enumeration::BlockCiphers::Rijndael,		// cipher engine
			Enumeration::BlockCipherExtensions::None,	// cipher extension
			Enumeration::CipherModes::CTR,				// cipher mode
			Enumeration::PaddingModes::None,			// padding mode
			Enumeration::KeySizes::K256,				// key size
			Enumeration::IVSizes::V128);				// cipher iv size

		CEX::IO::MemoryStream* ms = cd.ToStream();
		Processing::CipherDescription cy(*ms);
		delete ms;

		if (!cy.Equals(cd))
		{
			throw;
		}
	}

	void CipherStreamTest::StreamModesTest(Cipher::Symmetric::Block::Mode::ICipherMode* Cipher, Cipher::Symmetric::Block::Padding::IPadding* Padding)
	{
		Key::Symmetric::SymmetricKeySize keySize = Cipher->LegalKeySizes()[0];
		if (keySize.KeySize() > 32)
		{
			AllocateRandom(m_key, 192);
		}
		else
		{
			AllocateRandom(m_key, 32);
		}

		AllocateRandom(m_iv, 16);
		// we are testing padding modes, make sure input size is random, but -not- block aligned..
		AllocateRandom(m_plnText, 0, Cipher->BlockSize());

		Key::Symmetric::SymmetricKey kp(m_key, m_iv);
		IO::MemoryStream mIn(m_plnText);
		IO::MemoryStream mOut;
		IO::MemoryStream mRes;

		Processing::CipherStream cs(Cipher, Padding);
		cs.Initialize(true, kp);
		cs.Write(&mIn, &mOut);

		cs.Initialize(false, kp);
		mOut.Seek(0, IO::SeekOrigin::Begin);
		cs.Write(&mOut, &mRes);

		if (mRes.ToArray() != m_plnText)
		{
			throw TestException(std::string("CipherStreamTest: Encrypted arrays are not equal!"));
		}

		delete Cipher;
		delete Padding;
	}

	//~~~Helpers~~~//

	size_t CipherStreamTest::AllocateRandom(std::vector<byte> &Data, size_t Size, size_t NonAlign)
	{
		Prng::SecureRandom rng(Enumeration::Prngs::BCR, Enumeration::Providers::CSP);

		if (Size != 0)
		{
			Data.resize(Size);
		}
		else
		{
			size_t blkSze = 0;
			if (NonAlign != 0)
			{
				while ((blkSze = rng.NextUInt32(MAX_ALLOC, MIN_ALLOC)) % NonAlign == 0);
			}
			else
			{
				blkSze = rng.NextUInt32(MAX_ALLOC, MIN_ALLOC);
			}
			Data.resize(blkSze);
		}

		rng.Generate(Data);
		return (int)Data.size();
	}

	void CipherStreamTest::BlockCTR(Cipher::Symmetric::Block::Mode::ICipherMode* Cipher, const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
	{
		const size_t INPLEN = Input.size() - InOffset;
		Cipher->Transform(Input, InOffset, Output, OutOffset, INPLEN);
	}

	void CipherStreamTest::BlockDecrypt(Cipher::Symmetric::Block::Mode::ICipherMode* Cipher, Cipher::Symmetric::Block::Padding::IPadding* Padding, const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
	{
		const size_t BLKLEN = Cipher->BlockSize();
		const size_t INPLEN = Input.size() - InOffset;
		const size_t ALNLEN = (INPLEN < BLKLEN) ? 0 : ((INPLEN / BLKLEN) * BLKLEN) - BLKLEN;

		if (INPLEN > BLKLEN)
		{
			Cipher->ParallelProfile().IsParallel() = false;
			Cipher->Transform(Input, InOffset, Output, OutOffset, ALNLEN);
			InOffset += ALNLEN;
			OutOffset += ALNLEN;
		}

		// last block
		std::vector<byte> inpBuffer(BLKLEN);
		std::memcpy(&inpBuffer[0], &Input[InOffset], BLKLEN);
		std::vector<byte> outBuffer(BLKLEN);
		Cipher->DecryptBlock(inpBuffer, 0, outBuffer, 0);
		const size_t PADLEN = Padding->GetPaddingLength(outBuffer, 0);
		const size_t FNLLEN = (PADLEN == 0) ? BLKLEN : BLKLEN - PADLEN;
		std::memcpy(&Output[OutOffset], &outBuffer[0], FNLLEN);
		OutOffset += FNLLEN;

		if (Output.size() != OutOffset)
		{
			Output.resize(OutOffset);
		}
	}

	void CipherStreamTest::BlockEncrypt(Cipher::Symmetric::Block::Mode::ICipherMode* Cipher, Cipher::Symmetric::Block::Padding::IPadding* Padding, const std::vector<byte> &Input, size_t InOffset, std::vector<byte> &Output, size_t OutOffset)
	{
		const size_t BLKLEN = Cipher->BlockSize();
		const size_t INPLEN = Input.size() - InOffset;
		const size_t ALNLEN = (INPLEN < BLKLEN) ? 0 : INPLEN - (INPLEN % BLKLEN);

		if (INPLEN > BLKLEN)
		{
			Cipher->ParallelProfile().IsParallel() = false;
			Cipher->Transform(Input, InOffset, Output, OutOffset, ALNLEN);
			InOffset += ALNLEN;
			OutOffset += ALNLEN;
		}

		// partial
		if (ALNLEN != INPLEN)
		{
			size_t FNLLEN = INPLEN - ALNLEN;
			std::vector<byte> inpBuffer(BLKLEN);
			std::memcpy(&inpBuffer[0], &Input[InOffset], FNLLEN);
			if (FNLLEN != BLKLEN)
			{
				Padding->AddPadding(inpBuffer, FNLLEN);
			}
			std::vector<byte> outBuffer(BLKLEN);
			Cipher->EncryptBlock(inpBuffer, 0, outBuffer, 0);
			if (Output.size() != OutOffset + BLKLEN)
			{
				Output.resize(OutOffset + BLKLEN);
			}
			std::memcpy(&Output[OutOffset], &outBuffer[0], BLKLEN);
		}
	}

	void CipherStreamTest::OnProgress(std::string Data)
	{
		m_progressEvent(Data);
	}
}
