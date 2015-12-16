#include "garlic.h"
#include "global.h"

//////////////////////////////////////////
// generate four million keys and time it

void benchmark (HWND hWnd)                                                         // closely duplicates the actual search code for timing purposes
{
    int       pattern_len = 5;
    RSA       *rsa;
    char      onion[BASE32_ONIONLEN];
    BYTE      *tmp, len, der[RSA_KEYS_BITLEN], sha[SHA1_DIGEST_LEN], sha_pat[5] = {255,255,255,255,255};
    BYTE      exp[EXP_MAX_LEN] = {0, 0, 0, 0, 1}, e_ptr = EXP_MAX_LEN-1, e_len = 1, sha_tmp;
    SHA_CTX   hash, copy;
    clock_t   begin, end;
    double    elapsed;
    ULONGLONG loop = 4000000, e = 1;

start_new:

    rsa = RSA_generate_key(RSA_KEYS_BITLEN, RSA_PK_EXPONENT, NULL, NULL);

    begin = clock();                                                               // skip timing the first key as it unfairly impacts this quick benchmark
    tmp = der;
    len = i2d_RSAPublicKey(rsa, &tmp);
    SHA1_Init(&hash);
    SHA1_Update(&hash, der, 137);

    do
    {
      if (e == RSA_MAX_EXP) goto start_new;
      e += 2;
      exp[EXP_MAX_LEN-1] += 2;
      if (e==129 || e==32769 || e==8388609 || e==2147483649)
      {
        der[2]++;
        der[136]++;
        e_ptr--;
        e_len++;
        SHA1_Init(&hash);
        SHA1_Update(&hash, der, 137);
      }
      if (exp[EXP_MAX_LEN-1] == 1)
      {
        exp[EXP_MAX_LEN-2]++;
        if (exp[EXP_MAX_LEN-2] == 0)
        {
          exp[EXP_MAX_LEN-3]++;
          if (exp[EXP_MAX_LEN-3] == 0)
          {
            exp[EXP_MAX_LEN-4]++;
            if (exp[EXP_MAX_LEN-4] == 0)
            {
              exp[EXP_MAX_LEN-5]++;
            }
          }
        }
      }
      memcpy(&copy, &hash, SHA_REL_CTX_LEN);
      copy.num = hash.num;
      SHA1_Update(&copy, exp + e_ptr, e_len);
      SHA1_Final(sha, &copy);
      sha_tmp = sha[pattern_len-1];
      sha[pattern_len-1] = sha[pattern_len-1] >> 3;
      loop--;
    }
    while (memcmp(sha_pat, sha, pattern_len) && loop);                             // end of loop - will never find a match and so is
    end = clock();                                                                 // guaranteed to run the full, four million iterations.

    elapsed = difftime(end, begin);
    SpeedPerCore = .978 * 240000000000/elapsed;                                    // added a fudge factor to more closely approximate empirical results
    SHA1_Final(sha, &hash);                                                        // clean up context
    RSA_free(rsa);
    return;
}
