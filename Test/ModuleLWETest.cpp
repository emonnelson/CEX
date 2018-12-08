#include "ModuleLWETest.h"
#include "../CEX/CryptoAuthenticationFailure.h"
#include "../CEX/IAsymmetricKeyPair.h"
#include "../CEX/ModuleLWE.h"
#include "../CEX/MLWEKeyPair.h"
#include "../CEX/MLWEPrivateKey.h"
#include "../CEX/MLWEPublicKey.h"
#include "../CEX/RingLWE.h"

namespace Test
{
	using Enumeration::MLWEParameters;
	using namespace Key::Asymmetric;
	using namespace Cipher::Asymmetric::MLWE;

	const std::string ModuleLWETest::DESCRIPTION = "ModuleLWE key generation, encryption, and decryption tests..";
	const std::string ModuleLWETest::FAILURE = "FAILURE! ";
	const std::string ModuleLWETest::SUCCESS = "SUCCESS! ModuleLWE tests have executed succesfully.";

	ModuleLWETest::ModuleLWETest()
		:
		m_progressEvent(),
		m_rngPtr(new Prng::BCR)
	{
	}

	ModuleLWETest::~ModuleLWETest()
	{
		delete m_rngPtr;
	}

	const std::string ModuleLWETest::Description()
	{
		return DESCRIPTION;
	}

	TestEventHandler &ModuleLWETest::Progress()
	{
		return m_progressEvent;
	}

	std::string ModuleLWETest::Run()
	{
		try
		{
			Authentication();
			OnProgress(std::string("ModuleLWETest: Passed message authentication test.."));
			CipherText();
			OnProgress(std::string("ModuleLWETest: Passed cipher-text integrity test.."));
			Exception();
			OnProgress(std::string("ModuleLWETest: Passed exception handling test.."));
			PublicKey();
			OnProgress(std::string("ModuleLWETest: Passed public key integrity test.."));
			Serialization();
			OnProgress(std::string("ModuleLWETest: Passed key serialization tests.."));
			Stress();
			OnProgress(std::string("ModuleLWETest: Passed encryption and decryption stress tests.."));

			return SUCCESS;
		}
		catch (TestException const &ex)
		{
			throw TestException(FAILURE + std::string(" : ") + ex.Message());
		}
		catch (...)
		{
			throw TestException(FAILURE + std::string(" : Unknown Error"));
		}
	}

	void ModuleLWETest::Authentication()
	{
		std::vector<byte> cpt(0);
		std::vector<byte> sec1(32);
		std::vector<byte> sec2(32);

		// test param 1: MLWES2Q7681N256
		ModuleLWE cpr1(MLWEParameters::MLWES2Q7681N256, m_rngPtr);
		IAsymmetricKeyPair* kp1 = cpr1.Generate();

		cpr1.Initialize(kp1->PublicKey());
		cpr1.Encapsulate(cpt, sec1);

		// alter ciphertext
		m_rngPtr->Generate(cpt, 0, 4);

		cpr1.Initialize(kp1->PrivateKey());

		if (cpr1.Decapsulate(cpt, sec2))
		{
			throw TestException(std::string("ModuleLWE"), std::string("Message authentication test failed! -MA1"));
		}

		delete kp1;

		// test param 2: MLWES3Q7681N256
		ModuleLWE cpr2(MLWEParameters::MLWES3Q7681N256, m_rngPtr);
		IAsymmetricKeyPair* kp2 = cpr2.Generate();

		cpt.resize(0);

		cpr2.Initialize(kp2->PublicKey());
		cpr2.Encapsulate(cpt, sec1);

		// alter ciphertext
		m_rngPtr->Generate(cpt, 0, 4);

		cpr2.Initialize(kp2->PrivateKey());

		if (cpr2.Decapsulate(cpt, sec2))
		{
			throw TestException(std::string("ModuleLWE"), std::string("Message authentication test failed! -MA2"));
		}

		delete kp2;

		// test param 3: MLWES4Q7681N256
		ModuleLWE cpr3(MLWEParameters::MLWES3Q7681N256, m_rngPtr);
		IAsymmetricKeyPair* kp3 = cpr3.Generate();

		cpt.resize(0);

		cpr3.Initialize(kp3->PublicKey());
		cpr3.Encapsulate(cpt, sec1);

		// alter ciphertext
		m_rngPtr->Generate(cpt, 0, 4);

		cpr3.Initialize(kp3->PrivateKey());

		if (cpr3.Decapsulate(cpt, sec2))
		{
			throw TestException(std::string("ModuleLWE"), std::string("Message authentication test failed! -MA3"));
		}

		delete kp3;
	}

	void ModuleLWETest::CipherText()
	{
		std::vector<byte> cpt(0);
		std::vector<byte> sec1(64);
		std::vector<byte> sec2(64);

		// test param 1: MLWES2Q7681N256
		ModuleLWE cpr1(MLWEParameters::MLWES2Q7681N256, m_rngPtr);
		IAsymmetricKeyPair* kp1 = cpr1.Generate();

		cpr1.Initialize(kp1->PublicKey());
		cpr1.Encapsulate(cpt, sec1);

		// alter ciphertext
		m_rngPtr->Generate(cpt, 0, 4);

		cpr1.Initialize(kp1->PrivateKey());

		if (cpr1.Decapsulate(cpt, sec2))
		{
			throw TestException(std::string("ModuleLWE"), std::string("Cipher-text integrity test failed! -MC1"));
		}

		delete kp1;

		// test param 2: MLWES3Q7681N256
		ModuleLWE cpr2(MLWEParameters::MLWES3Q7681N256, m_rngPtr);
		IAsymmetricKeyPair* kp2 = cpr2.Generate();

		cpt.resize(0);

		cpr2.Initialize(kp2->PublicKey());
		cpr2.Encapsulate(cpt, sec1);

		// alter ciphertext
		m_rngPtr->Generate(cpt, 0, 4);

		cpr2.Initialize(kp2->PrivateKey());

		if (cpr2.Decapsulate(cpt, sec2))
		{
			throw TestException(std::string("ModuleLWE"), std::string("Cipher-text integrity test failed! -MC2"));
		}

		delete kp2;

		// test param 3: MLWES4Q7681N256
		ModuleLWE cpr3(MLWEParameters::MLWES4Q7681N256, m_rngPtr);
		IAsymmetricKeyPair* kp3 = cpr3.Generate();

		cpt.resize(0);

		cpr3.Initialize(kp3->PublicKey());
		cpr3.Encapsulate(cpt, sec1);

		// alter ciphertext
		m_rngPtr->Generate(cpt, 0, 4);

		cpr3.Initialize(kp3->PrivateKey());

		if (cpr3.Decapsulate(cpt, sec2))
		{
			throw TestException(std::string("ModuleLWE"), std::string("Cipher-text integrity test failed! -MC3"));
		}

		delete kp3;
	}

	void ModuleLWETest::Exception()
	{
		// test invalid constructor parameters
		try
		{
			ModuleLWE cpr(MLWEParameters::None, m_rngPtr);

			throw TestException(std::string("ModuleLWE"), std::string("Exception handling failure! -ME1"));
		}
		catch (CryptoAsymmetricException const &)
		{
		}
		catch (TestException const &)
		{
			throw;
		}

		try
		{
			ModuleLWE cpr(MLWEParameters::MLWES3Q7681N256, Enumeration::Prngs::None);

			throw TestException(std::string("ModuleLWE"), std::string("Exception handling failure! -ME2"));
		}
		catch (CryptoAsymmetricException const &)
		{
		}
		catch (TestException const &)
		{
			throw;
		}

		// test initialization
		try
		{
			ModuleLWE cpra(MLWEParameters::MLWES3Q7681N256, Enumeration::Prngs::BCR);
			Cipher::Asymmetric::RLWE::RingLWE cprb;
			// create an invalid key set
			IAsymmetricKeyPair* kp = cprb.Generate();
			cpra.Initialize(kp->PrivateKey());

			throw TestException(std::string("ModuleLWE"), std::string("Exception handling failure! -ME3"));
		}
		catch (CryptoAsymmetricException const &)
		{
		}
		catch (TestException const &)
		{
			throw;
		}
	}

	void ModuleLWETest::PublicKey()
	{
		std::vector<byte> cpt(0);
		std::vector<byte> sec1(64);
		std::vector<byte> sec2(64);

		// test param 1: MLWES2Q7681N256
		ModuleLWE cpr1(MLWEParameters::MLWES2Q7681N256, m_rngPtr);
		IAsymmetricKeyPair* kp1 = cpr1.Generate();

		// alter public key
		std::vector<byte> pk1 = ((MLWEPublicKey*)kp1->PublicKey())->P();
		pk1[0] += 1;
		pk1[1] += 1;
		MLWEPublicKey* pk2 = new MLWEPublicKey(MLWEParameters::MLWES2Q7681N256, pk1);
		cpr1.Initialize(pk2);
		cpr1.Encapsulate(cpt, sec1);

		cpr1.Initialize(kp1->PrivateKey());

		if (cpr1.Decapsulate(cpt, sec2))
		{
			throw TestException(std::string("ModuleLWE"), std::string("Public-key integrity test failed! -MP1"));
		}

		cpt.clear();
		sec1.clear();
		sec2.clear();
		cpt.resize(0);
		sec1.resize(64);
		sec2.resize(64);
		delete kp1;

		// test param 2: MLWES3Q7681N256
		ModuleLWE cpr2(MLWEParameters::MLWES3Q7681N256, m_rngPtr);
		IAsymmetricKeyPair* kp2 = cpr2.Generate();

		// alter public key
		std::vector<byte> pk3 = ((MLWEPublicKey*)kp2->PublicKey())->P();
		pk3[0] += 1;
		pk3[1] += 1;
		MLWEPublicKey* pk4 = new MLWEPublicKey(MLWEParameters::MLWES3Q7681N256, pk3);
		cpr2.Initialize(pk4);
		cpr2.Encapsulate(cpt, sec1);

		cpr2.Initialize(kp2->PrivateKey());

		if (cpr2.Decapsulate(cpt, sec2))
		{
			throw TestException(std::string("ModuleLWE"), std::string("Public-key integrity test failed! -MP2"));
		}

		cpt.clear();
		sec1.clear();
		sec2.clear();
		cpt.resize(0);
		sec1.resize(64);
		sec2.resize(64);
		delete kp2;

		// test param 3: MLWES4Q7681N256
		ModuleLWE cpr3(MLWEParameters::MLWES4Q7681N256, m_rngPtr);
		IAsymmetricKeyPair* kp3 = cpr3.Generate();

		// alter public key
		std::vector<byte> pk5 = ((MLWEPublicKey*)kp3->PublicKey())->P();
		pk5[0] += 1;
		pk5[1] += 1;
		MLWEPublicKey* pk6 = new MLWEPublicKey(MLWEParameters::MLWES4Q7681N256, pk5);
		cpr3.Initialize(pk6);
		cpr3.Encapsulate(cpt, sec1);

		cpr3.Initialize(kp3->PrivateKey());

		if (cpr3.Decapsulate(cpt, sec2))
		{
			throw TestException(std::string("ModuleLWE"), std::string("Public-key integrity test failed! -MP3"));
		}

		delete kp3;
	}

	void ModuleLWETest::Serialization()
	{
		std::vector<byte> skey;

		// test param 1: MLWES2Q7681N256
		ModuleLWE cpr1(MLWEParameters::MLWES2Q7681N256, m_rngPtr);

		for (size_t i = 0; i < TEST_CYCLES; ++i)
		{
			IAsymmetricKeyPair* kp = cpr1.Generate();
			MLWEPrivateKey* priK1 = (MLWEPrivateKey*)kp->PrivateKey();
			skey = priK1->ToBytes();
			MLWEPrivateKey priK2(skey);

			if (priK1->R() != priK2.R() || priK1->Parameters() != priK2.Parameters())
			{
				throw TestException(std::string("ModuleLWE"), std::string("Private key serialization test has failed! -MR1"));
			}

			MLWEPublicKey* pubK1 = (MLWEPublicKey*)kp->PublicKey();
			skey = pubK1->ToBytes();
			MLWEPublicKey pubK2(skey);

			if (pubK1->P() != pubK2.P() || pubK1->Parameters() != pubK2.Parameters())
			{
				throw TestException(std::string("ModuleLWE"), std::string("Public key serialization test has failed! -MR2"));
			}
		}

		skey.clear();
		skey.resize(0);

		// test param 2: MLWES3Q7681N256
		ModuleLWE cpr2(MLWEParameters::MLWES3Q7681N256, m_rngPtr);

		for (size_t i = 0; i < TEST_CYCLES; ++i)
		{

			IAsymmetricKeyPair* kp = cpr2.Generate();
			MLWEPrivateKey* priK1 = (MLWEPrivateKey*)kp->PrivateKey();
			skey = priK1->ToBytes();
			MLWEPrivateKey priK2(skey);

			if (priK1->R() != priK2.R() || priK1->Parameters() != priK2.Parameters())
			{
				throw TestException(std::string("ModuleLWE"), std::string("Private key serialization test has failed! -MR3"));
			}

			MLWEPublicKey* pubK1 = (MLWEPublicKey*)kp->PublicKey();
			skey = pubK1->ToBytes();
			MLWEPublicKey pubK2(skey);

			if (pubK1->P() != pubK2.P() || pubK1->Parameters() != pubK2.Parameters())
			{
				throw TestException(std::string("ModuleLWE"), std::string("Public key serialization test has failed! -MR4"));
			}
		}

		skey.clear();
		skey.resize(0);

		// test param 3: MLWES4Q7681N256
		ModuleLWE cpr3(MLWEParameters::MLWES4Q7681N256, m_rngPtr);

		for (size_t i = 0; i < TEST_CYCLES; ++i)
		{
			IAsymmetricKeyPair* kp = cpr3.Generate();
			MLWEPrivateKey* priK1 = (MLWEPrivateKey*)kp->PrivateKey();
			skey = priK1->ToBytes();
			MLWEPrivateKey priK2(skey);

			if (priK1->R() != priK2.R() || priK1->Parameters() != priK2.Parameters())
			{
				throw TestException(std::string("ModuleLWE"), std::string("Private key serialization test has failed! -MR5"));
			}

			MLWEPublicKey* pubK1 = (MLWEPublicKey*)kp->PublicKey();
			skey = pubK1->ToBytes();
			MLWEPublicKey pubK2(skey);

			if (pubK1->P() != pubK2.P() || pubK1->Parameters() != pubK2.Parameters())
			{
				throw TestException(std::string("ModuleLWE"), std::string("Public key serialization test has failed! -MR6"));
			}
		}
	}

	void ModuleLWETest::Stress()
	{
		std::vector<byte> cpt(0);
		std::vector<byte> sec1(32);
		std::vector<byte> sec2(32);

		ModuleLWE cpr1(MLWEParameters::MLWES3Q7681N256, m_rngPtr);

		for (size_t i = 0; i < TEST_CYCLES / 3; ++i)
		{
			m_rngPtr->Generate(sec1);
			IAsymmetricKeyPair* kp = cpr1.Generate();

			cpr1.Initialize(kp->PublicKey());
			cpr1.Encapsulate(cpt, sec1);

			cpr1.Initialize(kp->PrivateKey());

			if (!cpr1.Decapsulate(cpt, sec2))
			{
				throw TestException(std::string("ModuleLWE"), std::string("Stress test authentication has failed! -MT1"));
			}

			delete kp;

			if (sec1 != sec2)
			{
				throw TestException(std::string("ModuleLWE"), std::string("Stress test has failed! -MT2"));
			}
		}

		cpt.clear();
		sec1.clear();
		sec2.clear();
		cpt.resize(0);
		sec1.resize(64);
		sec2.resize(64);

		ModuleLWE cpr2(MLWEParameters::MLWES3Q7681N256, m_rngPtr);

		for (size_t i = 0; i < TEST_CYCLES / 3; ++i)
		{
			m_rngPtr->Generate(sec1);
			IAsymmetricKeyPair* kp = cpr2.Generate();

			cpr2.Initialize(kp->PublicKey());
			cpr2.Encapsulate(cpt, sec1);

			cpr2.Initialize(kp->PrivateKey());

			if (!cpr2.Decapsulate(cpt, sec2))
			{
				throw TestException(std::string("ModuleLWE"), std::string("Stress test authentication has failed! -MT3"));
			}

			delete kp;

			if (sec1 != sec2)
			{
				throw TestException(std::string("ModuleLWE"), std::string("Stress test has failed! -MT4"));
			}
		}

		cpt.clear();
		sec1.clear();
		sec2.clear();
		cpt.resize(0);
		sec1.resize(64);
		sec2.resize(64);

		ModuleLWE cpr3(MLWEParameters::MLWES4Q7681N256, m_rngPtr);

		for (size_t i = 0; i < TEST_CYCLES / 3; ++i)
		{
			m_rngPtr->Generate(sec1);
			IAsymmetricKeyPair* kp = cpr3.Generate();

			cpr3.Initialize(kp->PublicKey());
			cpr3.Encapsulate(cpt, sec1);

			cpr3.Initialize(kp->PrivateKey());

			if (!cpr3.Decapsulate(cpt, sec2))
			{
				throw TestException(std::string("ModuleLWE"), std::string("Stress test authentication has failed! -MT5"));
			}

			delete kp;

			if (sec1 != sec2)
			{
				throw TestException(std::string("ModuleLWE"), std::string("Stress test has failed! -MT6"));
			}
		}
	}

	void ModuleLWETest::OnProgress(std::string Data)
	{
		m_progressEvent(Data);
	}
}
