//
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Copyright © 2011-2019 ANSSI. All Rights Reserved.
//
// Author(s): Jean Gautier (ANSSI)
//
#include "stdafx.h"

#include "CryptoHashStream.h"
#include "MemoryStream.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Orc;
using namespace Orc::Test;

namespace Orc::Test {
TEST_CLASS(HashStreamTest)
{
private:
    UnitTestHelper helper;

public:
    TEST_METHOD_INITIALIZE(Initialize) {}

    TEST_METHOD_CLEANUP(Finalize) {}

    TEST_METHOD(HashStreamBasicTest)
    {
        // hash data
        {
            unsigned char data[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

            auto algs = CryptoHashStream::Algorithm::MD5 | CryptoHashStream::Algorithm::SHA1 | CryptoHashStream::Algorithm::SHA256;

            auto hashstream = std::make_shared<CryptoHashStream>();
            Assert::IsTrue(SUCCEEDED(hashstream->OpenToWrite(algs, nullptr)));

            ULONGLONG ullHashed = 0LL;
            Assert::IsTrue(SUCCEEDED(hashstream->Write(data, sizeof(data), &ullHashed)));

            {
                unsigned char md5Result[16] = {
                    0x37, 0xD3, 0xA1, 0x9F, 0xF4, 0x69, 0x7F, 0x91, 0xFD, 0xC4, 0xC3, 0xA0, 0x69, 0x0A, 0xD8, 0xC5};
                CBinaryBuffer md5;

                Assert::IsTrue(SUCCEEDED(hashstream->GetHash(CryptoHashStream::Algorithm::MD5, md5)));

                Assert::IsTrue(md5.GetCount() == sizeof(md5Result));
                Assert::IsTrue(!memcmp(md5.GetData(), md5Result, sizeof(md5Result)));
            }

            {
                unsigned char sha1Result[20] = {0xD5, 0xB4, 0x12, 0x31, 0x9A, 0x66, 0xBE, 0x9D, 0xCB, 0x40,
                                                0xEA, 0x0E, 0xE0, 0x9B, 0xB7, 0x62, 0x71, 0xC5, 0xE7, 0x48};
                CBinaryBuffer sha1;

                Assert::IsTrue(S_OK == hashstream->GetHash(CryptoHashStream::Algorithm::SHA1, sha1));

                Assert::IsTrue(sha1.GetCount() == sizeof(sha1Result));
                Assert::IsTrue(!memcmp(sha1.GetData(), sha1Result, sizeof(sha1Result)));
            }

            {
                unsigned char sha256Result[32] = {0xC7, 0x50, 0x4B, 0x38, 0xF7, 0xA3, 0x60, 0x63, 0x14, 0xFD, 0x11,
                                                  0xD5, 0x26, 0x88, 0x8D, 0x4E, 0xC4, 0x58, 0x4E, 0x9E, 0x08, 0xC8,
                                                  0x33, 0xC1, 0x25, 0x03, 0x8F, 0x2E, 0xB0, 0xDF, 0xD1, 0x6E};
                CBinaryBuffer sha256;

                Assert::IsTrue(S_OK == hashstream->GetHash(CryptoHashStream::Algorithm::SHA256, sha256));

                Assert::IsTrue(sha256.GetCount() == sizeof(sha256Result));
                Assert::IsTrue(!memcmp(sha256.GetData(), sha256Result, sizeof(sha256Result)));
            }
        }

        {
            unsigned char data[512] = {
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00};

            auto algs = CryptoHashStream::Algorithm::SHA256;

            auto hashstream = std::make_shared<CryptoHashStream>();
            Assert::IsTrue(S_OK == hashstream->OpenToWrite(algs, nullptr));

            ULONGLONG ullHashed = 0LL;
            Assert::IsTrue(S_OK == hashstream->Write(data, sizeof(data), &ullHashed));

            unsigned char sha256Result[32] = {0x07, 0x6A, 0x27, 0xC7, 0x9E, 0x5A, 0xCE, 0x2A, 0x3D, 0x47, 0xF9,
                                              0xDD, 0x2E, 0x83, 0xE4, 0xFF, 0x6E, 0xA8, 0x87, 0x2B, 0x3C, 0x22,
                                              0x18, 0xF6, 0x6C, 0x92, 0xB8, 0x9B, 0x55, 0xF3, 0x65, 0x60};
            CBinaryBuffer sha256;

            Assert::IsTrue(S_OK == hashstream->GetHash(CryptoHashStream::Algorithm::SHA256, sha256));

            Assert::IsTrue(sha256.GetCount() == sizeof(sha256Result));
            Assert::IsTrue(!memcmp(sha256.GetData(), sha256Result, sizeof(sha256Result)));
        }

        {
            // write & hash data
            unsigned char data[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

            auto pMemStream = std::make_shared<MemoryStream>();
            Assert::IsTrue(S_OK == pMemStream->OpenForReadWrite(sizeof(data)));

            auto hashstream = std::make_shared<CryptoHashStream>();
            Assert::IsTrue(
                S_OK == hashstream->OpenToWrite(CryptoHashStream::Algorithm::MD5, pMemStream));

            ULONGLONG ullHashed = 0LL;
            Assert::IsTrue(S_OK == hashstream->Write(data, sizeof(data), &ullHashed));

            CBinaryBuffer buffer;
            pMemStream->GrabBuffer(buffer);

            Assert::IsTrue(!memcmp(data, buffer.GetData(), sizeof(data)));

            unsigned char md5Result[16] = {
                0x37, 0xD3, 0xA1, 0x9F, 0xF4, 0x69, 0x7F, 0x91, 0xFD, 0xC4, 0xC3, 0xA0, 0x69, 0x0A, 0xD8, 0xC5};
            CBinaryBuffer md5;

            Assert::IsTrue(S_OK == hashstream->GetHash(CryptoHashStream::Algorithm::MD5, md5));

            Assert::IsTrue(md5.GetCount() == sizeof(md5Result));
            Assert::IsTrue(!memcmp(md5.GetData(), md5Result, sizeof(md5Result)));
        }

        {
            unsigned char data[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

            auto pMemStream = std::make_shared<MemoryStream>();
            Assert::IsTrue(S_OK == pMemStream->OpenForReadWrite(sizeof(data)));

            ULONGLONG ullDataWritten = 0LL;
            Assert::IsTrue(S_OK == pMemStream->CanWrite());
            Assert::IsTrue(S_OK == pMemStream->Write(data, sizeof(data), &ullDataWritten));
            Assert::IsTrue(ullDataWritten == 5);
            Assert::IsTrue(!memcmp(data, pMemStream->GetConstBuffer().GetData(), sizeof(data)));

            auto hashstream = std::make_shared<CryptoHashStream>();
            Assert::IsTrue(
                S_OK == hashstream->OpenToRead(CryptoHashStream::Algorithm::MD5, pMemStream));

            unsigned char readBuffer[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
            ULONGLONG ullDataRead = 0LL;
            Assert::IsTrue(S_OK == hashstream->CanSeek());
            Assert::IsTrue(S_OK == hashstream->SetFilePointer(0LL, FILE_BEGIN, NULL));
            Assert::IsTrue(S_OK == hashstream->CanRead());
            Assert::IsTrue(S_OK == hashstream->Read(readBuffer, sizeof(readBuffer), &ullDataRead));
            Assert::IsTrue(ullDataRead == 5);

            unsigned char md5Result[16] = {
                0x37, 0xD3, 0xA1, 0x9F, 0xF4, 0x69, 0x7F, 0x91, 0xFD, 0xC4, 0xC3, 0xA0, 0x69, 0x0A, 0xD8, 0xC5};
            CBinaryBuffer md5;

            Assert::IsTrue(S_OK == hashstream->GetHash(CryptoHashStream::Algorithm::MD5, md5));

            Assert::IsTrue(md5.GetCount() == sizeof(md5Result));
            Assert::IsTrue(!memcmp(md5.GetData(), md5Result, sizeof(md5Result)));
        }
    }
};
}  // namespace Orc::Test
