#include "garlic.h"
#include "global.h"

///////////////////////////////////////////////////////////
// Search thread - idle priority

void thread (PVOID pvoid)
{
    FILE            *file;
    int             i, j, pattern_len = strlen(szPattern);
    RSA             *rsa;
    BUF_MEM         *buf;
    BIO             *b;
    DWORD           dwTimeout;
    char            dst[911], onion[BASE32_ONIONLEN];
    TCHAR           buffer[16];
    BYTE            *tmp, len, der[RSA_KEYS_BITLEN], sha[SHA1_DIGEST_LEN];
    BYTE            exp[EXP_MAX_LEN], e_ptr, e_len, sha_tmp;
    SHA_CTX         hash, copy;
    BN_CTX          *ctx;
    BIGNUM          *phi, *p;
    BIGNUM          *chk, *q;
    clock_t         begin, end;
    double          elapsed, TriesPerMinute;
    PPARAMS         pparams;
    HWND            hWndAnimate, hWndRandom, hWndThreads;
    NOTIFYICONDATA  nid;
    WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
    ULONGLONG       loop = 0, e;

    pparams = (PPARAMS) pvoid;
    pparams->bContinue = TRUE;

    nid.cbSize = sizeof(NOTIFYICONDATA); 
    nid.hWnd = pparams->hWnd; 
    nid.uID = IDI_TRAYICON; 

    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dwTimeout, 0);
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)0, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);

    begin = clock();

start_new:

    e = 1;                                                                         // exponent will increment to 3 before first hash is processed
    exp[EXP_MAX_LEN-5] = 0;                                                        // initialize our 5-byte exponent array to 0, 0, 0, 0, 1
    exp[EXP_MAX_LEN-4] = 0;
    exp[EXP_MAX_LEN-3] = 0;
    exp[EXP_MAX_LEN-2] = 0;
    exp[EXP_MAX_LEN-1] = 1;
    e_ptr = EXP_MAX_LEN-1;                                                         // the pointer to the most significant byte
    e_len = 1;                                                                     // the length of the exponent

    rsa = RSA_generate_key(RSA_KEYS_BITLEN, e, NULL, NULL);                        // openSSL gives us a good starting key
    tmp = der;                                                                     // a 'feature' of DER encoding is that the pointer gets modified
    len = i2d_RSAPublicKey(rsa, &tmp);                                             // so use our throw-away pointer for the encoding
    SHA1_Init(&hash);                                                              // get a context for our digest
    SHA1_Update(&hash, der, 137);                                                  // prehash the fixed portion of the DER encoding

    do                                                                             // only what is absolutely necessary here
    {
      if (e == RSA_MAX_EXP) goto start_new;                                        // out of exponent space - get a new starting key (19 lines up)
      e += 2;                                                                      // increment the exponent by 2 each time to keep it odd
      exp[EXP_MAX_LEN-1] += 2;                                                     // increment our own exponent array equivalent to a DER encoded one
      if (e==129 || e==32769 || e==8388609 || e==2147483649)                       // at these 4 odd times the number of exponent bytes increases
      {
        der[2]++;                                                                  // der[2] holds the number of bytes to follow
        der[136]++;                                                                // der[136] holds the number of exponent bytes
        e_ptr--;
        e_len++;
        SHA1_Init(&hash);                                                          // freshen the context and
        SHA1_Update(&hash, der, 137);                                              // prehash the fixed portion of the DER encoding again
      }
      if (exp[EXP_MAX_LEN-1] == 1)                                                 // implement our own add with carry
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
      memcpy(&copy, &hash, SHA_REL_CTX_LEN);                                       // copy the relevant parts of our already set up context (40 bytes)
      copy.num = hash.num;                                                         // and don't forget the num (9)
      SHA1_Update(&copy, exp + e_ptr, e_len);                                      // hashing only the exponent saves a ton of time and it's
      SHA1_Final(sha, &copy);                                                      // done on the copy so the first 137 bytes are untouched
      sha_tmp = sha[pparams->iSha];                                                // save, then shift the last byte of the pattern we seek
      sha[pparams->iSha] = sha[pparams->iSha] >> pparams->iShift;
      loop++;
    }
    while (memcmp(pparams->sha_pat, sha, pparams->iSha+1) && pparams->bContinue);
    end = clock();

    if (pparams->bContinue)                                                        // this gets executed by the winning thread only
    {
    pparams->bContinue = FALSE;                                                    // kill any other running threads
    KillTimer(pparams->hWnd, 1);
    sha[pparams->iSha] = sha_tmp;                                                  // restore the byte we shifted
    BN_bin2bn(exp + e_ptr, e_len, rsa->e);                                         // place the winning exponent into the RSA context

    ctx = BN_CTX_new();                                                            // now check our key for sanity
    BN_CTX_start(ctx);
    phi = BN_CTX_get(ctx);
    p = BN_CTX_get(ctx);
    chk = BN_CTX_get(ctx);
    q = BN_CTX_get(ctx);
    BN_sub(p, rsa->p, BN_value_one());
    BN_sub(q, rsa->q, BN_value_one());
    BN_mul(phi, p, q, ctx);

    BN_gcd(chk, phi, rsa->e, ctx);                                                 // check if public exponent e is coprime to phi(n)
    if (!BN_is_one(chk))
    SetDlgItemText(pparams->hWnd, IDC_PATTERN, "unsane key");

    BN_sub(chk, phi, rsa->e);                                                      // check if public exponent e is less than phi(n)
    if (chk->neg)
    SetDlgItemText(pparams->hWnd, IDC_PATTERN, "unsane key");

    BN_mod_inverse(rsa->d, rsa->e, phi, ctx);
    BN_mod(rsa->dmp1, rsa->d, p, ctx);
    BN_mod(rsa->dmq1, rsa->d, q, ctx);
    BN_mod_inverse(rsa->iqmp, rsa->q, rsa->p, ctx);
    BN_CTX_end(ctx); BN_CTX_free(ctx);

    if (RSA_check_key(rsa) != 1)                                                   // ask openSSL to check our key also
    SetDlgItemText(pparams->hWnd, IDC_PATTERN, "unsane key");

    SetDlgItemText(pparams->hWnd, IDC_ENTER_NAME, "Enter name [a-z] && [2-7] only");
    elapsed = difftime(end, begin);

    if (!elapsed)                                                                  // avoid division by zero
    {
      sprintf(szBuffer, "Time elapsed: 0.00 seconds   Tries per minute: %.2f million", pparams->iThreadCount*SpeedPerCore/1000000);
    }
    else
    {
      TriesPerMinute = 60 * loop * pparams->iThreadCount / (elapsed * 1000);
      if (elapsed < 60000)
      {
        sprintf(szBuffer, "Time elapsed: %.2f seconds   Tries per minute: %.2f million", elapsed/1000, TriesPerMinute);
      }
      if (elapsed >= 60000)
      {
        sprintf(szBuffer, "Time elapsed: %.2f minutes   Tries per minute: %.2f million", elapsed/60000, TriesPerMinute);
      }
      if (elapsed >= 3600000)
      {
        sprintf(szBuffer, "Time elapsed: %.2f hours   Tries per minute: %.2f million", elapsed/3600000, TriesPerMinute);
      }
      if (elapsed >= 86400000)
      {
        sprintf(szBuffer, "Time elapsed: %.2f days   Tries per minute: %.2f million", elapsed/86400000, TriesPerMinute);
      }
    }
    SetDlgItemText(pparams->hWnd, IDC_TRIES, szBuffer);
    base32_enc(onion, sha);                                                        // represent the first 80-bits of the SHA1 hash as base32
    sprintf(szFullname, "%s.onion\r\n", onion);                                    // append ".onion" and print it
    SetDlgItemText(pparams->hWnd, IDC_ONION_NAME, szFullname);
    b = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPrivateKey(b, rsa, NULL, NULL, 0, NULL, NULL);                // PEM format the private key
    BIO_get_mem_ptr(b, &buf);
    BIO_set_close(b, BIO_NOCLOSE);
    BIO_free(b);
    for (i=0, j=0; i < buf->length; i++, j++)                                      // add carriage returns (for windows)
    {
      if (buf->data[i] == '\n')
      {
        dst[j] = '\r';
        j++;
      }
      dst[j] = buf->data[i];
    }
    dst[buf->length+j-i] = '\0';
    SetDlgItemText(pparams->hWnd, IDC_RSA_KEY, dst);                               // print the PEM formatted private key
    BUF_MEM_free(buf);                                                             // clean up after ourselves
    RSA_free(rsa);

    GetPrivateProfileString("Settings", "Logging", "1", buffer, 16, szIniFilePath);
    if (pattern_len > 5 && (buffer[0] > '0'))                                      // optionally log results of 6 or more characters
    {
      if (buffer[0] > '1')
      {
        CreateDirectory(szLogFilePath, NULL);
        sprintf(szBuffer, "%s%s%s", szLogFilePath, szPattern, ".log");
      }
      else
      {
        lstrcpy(szBuffer, "garlic.log");
      }
      if (file = fopen (szBuffer, "ab"))
      {
        fprintf (file, "%s\r\n", szFullname) ;
        fprintf (file, "%s\r\n\r\n", dst) ;
        fclose (file);
      }
    }
    hWndAnimate=GetDlgItem (pparams->hWnd, IDC_ANIMATE);                           // stop the animation
    ShowWindow (hWndAnimate, SW_HIDE);
    hWndRandom=GetDlgItem (pparams->hWnd, IDC_RANDOM);
    ShowWindow (hWndRandom, SW_SHOW);

    GetWindowPlacement (pparams->hWnd, &wp);                                       // bring the minimized window into the foreground
    if (wp.showCmd != SW_SHOWNORMAL)
      {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        ShowWindow (pparams->hWnd, SW_SHOWNORMAL);
        SetForegroundWindow (pparams->hWnd);
      }
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)dwTimeout, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
    hWndThreads = GetDlgItem(pparams->hWnd, IDC_THREADS);
    SendMessage(hWndThreads, EM_SETREADONLY, FALSE, NULL);
    }                                                                              // end of winning thread
    SHA1_Final(sha, &hash);                                                        // clean up the unfinalized context in all threads
    _endthread();
}
