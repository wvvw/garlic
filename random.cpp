#include "garlic.h"
#include "global.h"

/////////////////////////////////////////
// generate a random onion & private key

void random (HWND hWnd)                                                            // Minimum required steps to generate and display a key via openSSL
{
    int     i, j;
    RSA     *rsa;
    BUF_MEM *buf;
    BIO     *b;
    char    dst[911], onion[BASE32_ONIONLEN];
    BYTE    *tmp, len, der[RSA_KEYS_BITLEN], sha[SHA1_DIGEST_LEN];
    SHA_CTX hash;

    rsa = RSA_generate_key(RSA_KEYS_BITLEN, RSA_PK_EXPONENT, NULL, NULL);          // Ask openSSL to generate a key pair
    tmp = der;
    len = i2d_RSAPublicKey(rsa, &tmp);                                             // DER encode the public key using a throwaway pointer
    SHA1_Init(&hash);
    SHA1_Update(&hash, der, len);                                                  // SHA1 hash the DER encoding
    SHA1_Final(sha, &hash);
    base32_enc(onion, sha);                                                        // Represent the first 80 bits of the SHA1 hash in base32
    sprintf(szFullname, "%s.onion\r\n", onion);
    SetDlgItemText(hWnd, IDC_ONION_NAME, szFullname);                              // Append ".onion" and print it
    b = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPrivateKey(b, rsa, NULL, NULL, 0, NULL, NULL);                // PEM format the private key
    BIO_get_mem_ptr(b, &buf);
    BIO_set_close(b, BIO_NOCLOSE);
    BIO_free(b);
    for (i=0, j=0; i < buf->length; i++, j++)                                      // Add carriage returns to the PEM formatted key (for windows)
    {
      if (buf->data[i] == '\n')
      {
        dst[j] = '\r';
        j++;
      }
      dst[j] = buf->data[i];
    }
    dst[buf->length+j-i] = '\0';
    SetDlgItemText(hWnd, IDC_RSA_KEY, dst);                                        // Print the private key
    BUF_MEM_free(buf);
    RSA_free(rsa);
    return;
}
