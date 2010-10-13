/*
* TEA
* (C) 1999-2007 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/tea.h>
#include <botan/loadstor.h>

namespace Botan {

/*
* TEA Encryption
*/
void TEA::encrypt_n(const byte in[], byte out[], size_t blocks) const
   {
   for(size_t i = 0; i != blocks; ++i)
      {
      u32bit L = load_be<u32bit>(in, 0);
      u32bit R = load_be<u32bit>(in, 1);

      u32bit S = 0;
      for(size_t j = 0; j != 32; ++j)
         {
         S += 0x9E3779B9;
         L += ((R << 4) + K[0]) ^ (R + S) ^ ((R >> 5) + K[1]);
         R += ((L << 4) + K[2]) ^ (L + S) ^ ((L >> 5) + K[3]);
         }

      store_be(out, L, R);

      in += block_size();
      out += block_size();
      }
   }

/*
* TEA Decryption
*/
void TEA::decrypt_n(const byte in[], byte out[], size_t blocks) const
   {
   for(size_t i = 0; i != blocks; ++i)
      {
      u32bit L = load_be<u32bit>(in, 0);
      u32bit R = load_be<u32bit>(in, 1);

      u32bit S = 0xC6EF3720;
      for(size_t j = 0; j != 32; ++j)
         {
         R -= ((L << 4) + K[2]) ^ (L + S) ^ ((L >> 5) + K[3]);
         L -= ((R << 4) + K[0]) ^ (R + S) ^ ((R >> 5) + K[1]);
         S -= 0x9E3779B9;
         }

      store_be(out, L, R);

      in += block_size();
      out += block_size();
      }
   }

/*
* TEA Key Schedule
*/
void TEA::key_schedule(const byte key[], size_t)
   {
   for(size_t i = 0; i != 4; ++i)
      K[i] = load_be<u32bit>(key, i);
   }

}
