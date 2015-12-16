#pragma warning (disable:4996)  // use of deprecated - want this to compile on earlier compilers too.

#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlobj.h>
#include <process.h>
#include <time.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/pem.h>

#define IDI_TRAYICON     100
#define IDD_DIALOG1      101
#define IDM_TRAYEXIT     102
#define IDM_TRAYRESTORE  103

#define IDC_STATIC        -1
#define IDC_ENTER_NAME  1000
#define IDC_PATTERN     1001
#define IDC_RANDOM      1002
#define IDC_EXIT        1003
#define IDC_PROJECTED   1004
#define IDC_TIME        1005
#define IDC_HOSTNAME    1006
#define IDC_ONION_NAME  1007
#define IDC_SETTINGS    1008
#define IDC_SAVE        1009
#define IDC_PRIVATE_KEY 1010
#define IDC_TRIES       1011
#define IDC_RSA_KEY     1012
#define IDC_ANIMATE     1013
#define IDC_NUMBER      1014
#define IDC_THREADS     1015
#define IDC_UPDOWN      1016
#define ID_BENCHMARK    1017
#define ID_THREADS      1018
#define ID_SEARCH       1019
#define ID_PATH         1020
#define ID_POSITION     1021
#define ID_LOG0         1022
#define ID_LOG1         1023
#define ID_LOG2         1024

#define WM_TRAYICON (WM_USER+1)

#define RSA_KEYS_BITLEN 1024
#define RSA_PK_EXPONENT 65537
#define RSA_MAX_EXP     1099511627775
#define SHA_REL_CTX_LEN 10 * sizeof(SHA_LONG)
#define SHA_PATTERN_LEN 10
#define EXP_MAX_LEN     5

#define BORDER_BAR      21
#define SHA1_DIGEST_LEN 20
#define BASE32_ONIONLEN SHA1_DIGEST_LEN/2*8/5+1

#define BASE32_ALPHABET "abcdefghijklmnopqrstuvwxyz234567"

#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0x0040
#endif

typedef struct
  {
  HWND hWnd;
  BOOL bContinue;
  int  iThreadCount, iSha, iShift;
  BYTE sha_pat[10];
  }
  PARAMS, *PPARAMS;

void   random (HWND);
void   benchmark (HWND);
void   PrintProjected (HWND);
void   base32_dec (BYTE*, char*);
void   base32_enc (char*, BYTE*);
void   thread (PVOID);
static UINT TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");

BOOL   CALLBACK SettingsDlgProc (HWND, UINT, WPARAM, LPARAM);
