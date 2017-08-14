/*
* (C) 2014,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "tests.h"

#if defined(BOTAN_HAS_AEAD_OCB)
   #include <botan/ocb.h>
   #include <botan/loadstor.h>
#endif

namespace Botan_Tests {

namespace {

#if defined(BOTAN_HAS_AEAD_OCB)

// Toy cipher used for wide block tests

class OCB_Wide_Test_Block_Cipher : public Botan::BlockCipher
   {
   public:
      OCB_Wide_Test_Block_Cipher(size_t bs) : m_bs(bs) {}

      std::string name() const override { return "OCB_ToyCipher"; }
      size_t block_size() const override { return m_bs; }
      void clear() override { m_key.clear(); }

      BlockCipher* clone() const { return new OCB_Wide_Test_Block_Cipher(m_bs); }

      void key_schedule(const uint8_t key[], size_t length) override
         {
         m_key.assign(key, key + length);
         }

      Botan::Key_Length_Specification key_spec() const override
         {
         return Botan::Key_Length_Specification(m_bs);
         }

      void encrypt_n(const uint8_t in[], uint8_t out[],
                     size_t blocks) const override
         {
         using namespace Botan;

         while(blocks)
            {
            const uint8_t top_carry = in[0] >> 7;
            uint8_t carry = 0;
            for(size_t i = m_bs; i != 0; --i)
               {
               uint8_t temp = in[i-1];
               out[i-1] = (temp << 1) | carry;
               carry = (temp >> 7);
               }

            if(top_carry)
               {
               if(m_bs == 16 || m_bs == 24)
                  {
                  out[m_bs-1] ^= 0x87;
                  }
               else if(m_bs == 32)
                  {
                  out[m_bs-2] ^= 0x4;
                  out[m_bs-1] ^= 0x25;
                  }
               else if(m_bs == 64)
                  {
                  out[m_bs-2] ^= 0x1;
                  out[m_bs-1] ^= 0x25;
                  }
               else
                  throw Test_Error("Bad OCB test block size");
               }

            for(size_t i = 0; i != m_bs; ++i)
               out[i] ^= m_key[i];

            blocks--;
            in += block_size();
            out += block_size();
            }
         }

      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override
         {
         while(blocks)
            {
            for(size_t i = 0; i != m_bs; ++i)
               out[i] = in[i] ^ m_key[i];

            const uint8_t bottom_carry = in[m_bs-1] & 0x01;

            if(bottom_carry)
               {
               if(m_bs == 16 || m_bs == 24)
                  {
                  out[m_bs-1] ^= 0x87;
                  }
               else if(m_bs == 32)
                  {
                  out[m_bs-2] ^= 0x4;
                  out[m_bs-1] ^= 0x25;
                  }
               else if(m_bs == 64)
                  {
                  out[m_bs-2] ^= 0x1;
                  out[m_bs-1] ^= 0x25;
                  }
               else
                  throw Test_Error("Bad OCB test block size");
               }

            uint8_t carry = bottom_carry << 7;

            for(size_t i = 0; i != m_bs; ++i)
               {
               uint8_t temp = out[i];
               out[i] = (temp >> 1) | carry;
               carry = (temp & 0x1) << 7;
               }

            blocks--;
            in += block_size();
            out += block_size();
            }
         }
   private:
      size_t m_bs;
      std::vector<uint8_t> m_key;
   };

class OCB_Wide_KAT_Tests : public Text_Based_Test
   {
   public:
      OCB_Wide_KAT_Tests()
         : Text_Based_Test("ocb_wide.vec", "Key,Nonce,AD,In,Out") {}

      Test::Result run_one_test(const std::string&, const VarMap& vars) override
         {
         Test::Result result("OCB wide block KAT");

         const std::vector<uint8_t> key = get_req_bin(vars, "Key");
         const std::vector<uint8_t> nonce = get_req_bin(vars, "Nonce");
         const std::vector<uint8_t> ad = get_req_bin(vars, "AD");
         const std::vector<uint8_t> input = get_req_bin(vars, "In");
         const std::vector<uint8_t> expected = get_req_bin(vars, "Out");

         const size_t bs = key.size();
         Botan::secure_vector<uint8_t> buf(input.begin(), input.end());

         Botan::OCB_Encryption enc(new OCB_Wide_Test_Block_Cipher(bs), std::min<size_t>(bs, 32));
         enc.set_key(key);
         enc.set_ad(ad);
         enc.start(nonce);
         enc.finish(buf);
         result.test_eq("Ciphertext matches", buf, expected);

         Botan::OCB_Decryption dec(new OCB_Wide_Test_Block_Cipher(bs), std::min<size_t>(bs, 32));
         dec.set_key(key);
         dec.set_ad(ad);
         dec.start(nonce);
         dec.finish(buf);
         result.test_eq("Decryption correct", buf, input);

         return result;
         }
   };

BOTAN_REGISTER_TEST("ocb_wide", OCB_Wide_KAT_Tests);

class OCB_Wide_Long_KAT_Tests : public Text_Based_Test
   {
   public:
      OCB_Wide_Long_KAT_Tests()
         : Text_Based_Test("ocb_wide_long.vec", "Blocklen,Output") {}

      Test::Result run_one_test(const std::string&, const VarMap& vars) override
         {
         Test::Result result("OCB wide block long test");

         const size_t bs = get_req_sz(vars, "Blocklen") / 8;
         const std::vector<uint8_t> expected = get_req_bin(vars, "Output");

         if(bs != 16 && bs != 32 && bs != 64)
            throw Test_Error("Unsupported Blocklen in OCB wide block test");

         Botan::OCB_Encryption enc(new OCB_Wide_Test_Block_Cipher(bs), std::min<size_t>(bs, 32));

         /*
         Y, string of length min(B, 256) bits

         Y is defined as follows.

         K = (0xA0 || 0xA1 || 0xA2 || ...)[1..B]
         C = <empty string>
         for i = 0 to 127 do
           S = (0x50 || 0x51 || 0x52 || ...)[1..8i]
           N = num2str(3i+1,16)
           C = C || OCB-ENCRYPT(K,N,S,S)
           N = num2str(3i+2,16)
           C = C || OCB-ENCRYPT(K,N,<empty string>,S)
           N = num2str(3i+3,16)
           C = C || OCB-ENCRYPT(K,N,S,<empty string>)
         end for
         N = num2str(385,16)
         Y = OCB-ENCRYPT(K,N,C,<empty string>)
         */

         std::vector<uint8_t> key(bs);
         for(size_t i = 0; i != bs; ++i)
            key[i] = 0xA0 + i;

         enc.set_key(key);

         const std::vector<uint8_t> empty;
         std::vector<uint8_t> N(2);
         std::vector<uint8_t> C;

         for(size_t i = 0; i != 128; ++i)
            {
            const std::vector<uint8_t> S(i);

            Botan::store_be(static_cast<uint32_t>(3 * i + 1), &N[8]);

            ocb_encrypt(result, C, enc, N, S, S);
            Botan::store_be(static_cast<uint32_t>(3 * i + 2), &N[8]);
            ocb_encrypt(result, C, enc, N, S, empty);
            Botan::store_be(static_cast<uint32_t>(3 * i + 3), &N[8]);
            ocb_encrypt(result, C, enc, N, empty, S);
            }

         Botan::store_be(static_cast<uint32_t>(385), &N[8]);
         std::vector<uint8_t> final_result;
         ocb_encrypt(result, final_result, enc, N, empty, C);

         result.test_eq("correct value", final_result, expected);

         return result;
         }

   private:
      void ocb_encrypt(Test::Result& result,
                       std::vector<uint8_t>& output_to,
                       Botan::OCB_Encryption& enc,
                       const std::vector<uint8_t>& nonce,
                       const std::vector<uint8_t>& pt,
                       const std::vector<uint8_t>& ad)
         {
         enc.set_associated_data(ad.data(), ad.size());
         enc.start(nonce.data(), nonce.size());
         Botan::secure_vector<uint8_t> buf(pt.begin(), pt.end());
         enc.finish(buf, 0);
         output_to.insert(output_to.end(), buf.begin(), buf.end());
         }
   };

BOTAN_REGISTER_TEST("ocb_long_wide", OCB_Wide_Long_KAT_Tests);

class OCB_Long_KAT_Tests : public Text_Based_Test
   {
   public:
      OCB_Long_KAT_Tests()
         : Text_Based_Test("ocb_long.vec",
                           "Keylen,Taglen,Output") {}

      Test::Result run_one_test(const std::string&, const VarMap& vars) override
         {
         const size_t keylen = get_req_sz(vars, "Keylen");
         const size_t taglen = get_req_sz(vars, "Taglen");
         const std::vector<uint8_t> expected = get_req_bin(vars, "Output");

         // Test from RFC 7253 Appendix A

         const std::string algo = "AES-" + std::to_string(keylen);

         Test::Result result("OCB long");

         std::unique_ptr<Botan::BlockCipher> aes(Botan::BlockCipher::create(algo));
         if(!aes)
            {
            result.note_missing(algo);
            return result;
            }

         Botan::OCB_Encryption enc(aes->clone(), taglen / 8);
         Botan::OCB_Decryption dec(aes->clone(), taglen / 8);

         std::vector<uint8_t> key(keylen / 8);
         key[keylen / 8 - 1] = taglen;

         enc.set_key(key);
         dec.set_key(key);

         const std::vector<uint8_t> empty;
         std::vector<uint8_t> N(12);
         std::vector<uint8_t> C;

         for(size_t i = 0; i != 128; ++i)
            {
            const std::vector<uint8_t> S(i);

            Botan::store_be(static_cast<uint32_t>(3 * i + 1), &N[8]);

            ocb_encrypt(result, C, enc, dec, N, S, S);
            Botan::store_be(static_cast<uint32_t>(3 * i + 2), &N[8]);
            ocb_encrypt(result, C, enc, dec, N, S, empty);
            Botan::store_be(static_cast<uint32_t>(3 * i + 3), &N[8]);
            ocb_encrypt(result, C, enc, dec, N, empty, S);
            }

         Botan::store_be(static_cast<uint32_t>(385), &N[8]);
         std::vector<uint8_t> final_result;
         ocb_encrypt(result, final_result, enc, dec, N, empty, C);

         result.test_eq("correct value", final_result, expected);

         return result;
         }
   private:
      void ocb_encrypt(Test::Result& result,
                       std::vector<uint8_t>& output_to,
                       Botan::OCB_Encryption& enc,
                       Botan::OCB_Decryption& dec,
                       const std::vector<uint8_t>& nonce,
                       const std::vector<uint8_t>& pt,
                       const std::vector<uint8_t>& ad)
         {
         enc.set_associated_data(ad.data(), ad.size());

         enc.start(nonce.data(), nonce.size());

         Botan::secure_vector<uint8_t> buf(pt.begin(), pt.end());
         enc.finish(buf, 0);
         output_to.insert(output_to.end(), buf.begin(), buf.end());

         try
            {
            dec.set_associated_data(ad.data(), ad.size());

            dec.start(nonce.data(), nonce.size());

            dec.finish(buf, 0);

            result.test_eq("OCB round tripped", buf, pt);
            }
         catch(std::exception& e)
            {
            result.test_failure("OCB round trip error", e.what());
            }

         }
   };

BOTAN_REGISTER_TEST("ocb_long", OCB_Long_KAT_Tests);

#endif

}

}
