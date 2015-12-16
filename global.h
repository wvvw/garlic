////////////////////////////////////////////////////
// Globals

extern HINSTANCE hInst;
extern TCHAR     szCurDir[_MAX_PATH];
extern TCHAR     szFileTitle[_MAX_FNAME + _MAX_EXT];
extern TCHAR     szPattern[BASE32_ONIONLEN];
extern TCHAR     szFullname[BASE32_ONIONLEN + 8];
extern TCHAR     szIniFilePath[_MAX_PATH];
extern TCHAR     szLogFilePath[_MAX_PATH];
extern TCHAR     szBuffer[_MAX_FNAME + _MAX_EXT];                                  // for general use
extern PARAMS    params;
extern double    SpeedPerCore;
extern clock_t   tBegin;
