#include "garlic.h"
#include "global.h"

///////////////////////////////////////////////////////////
// Globals

HINSTANCE hInst;
TCHAR     szCurDir[_MAX_PATH];
TCHAR     szFileTitle[_MAX_FNAME + _MAX_EXT];
TCHAR     szPattern[BASE32_ONIONLEN];
TCHAR     szFullname[BASE32_ONIONLEN + 8];
TCHAR     szIniFilePath[_MAX_PATH];
TCHAR     szLogFilePath[_MAX_PATH];
TCHAR     szBuffer[_MAX_FNAME + _MAX_EXT];                                         // general use buffer
char      dst[911];                                                                // serves double duty here
PARAMS    params;                                                                  // to pass parameters to and from the threads
double    SpeedPerCore;
clock_t   tBegin;                                                                  // for the timer

//////////////////////////////////////////////////
// Convert SHA1 hash of DER encoded key to base32                                  // Thanks to Unperson Hiro for this routine

void base32_enc (char *dst, BYTE *src)
{
	dst[ 0] = BASE32_ALPHABET[ (src[0] >> 3)			    ];
	dst[ 1] = BASE32_ALPHABET[((src[0] << 2) | (src[1] >> 6))	& 31];
	dst[ 2] = BASE32_ALPHABET[ (src[1] >> 1) 			& 31];
	dst[ 3] = BASE32_ALPHABET[((src[1] << 4) | (src[2] >> 4))	& 31];
	dst[ 4] = BASE32_ALPHABET[((src[2] << 1) | (src[3] >> 7))	& 31];
	dst[ 5] = BASE32_ALPHABET[ (src[3] >> 2)			& 31];
	dst[ 6] = BASE32_ALPHABET[((src[3] << 3) | (src[4] >> 5))	& 31];
	dst[ 7] = BASE32_ALPHABET[  src[4]				& 31];

	dst[ 8] = BASE32_ALPHABET[ (src[5] >> 3)			    ];
	dst[ 9] = BASE32_ALPHABET[((src[5] << 2) | (src[6] >> 6))	& 31];
	dst[10] = BASE32_ALPHABET[ (src[6] >> 1)			& 31];
	dst[11] = BASE32_ALPHABET[((src[6] << 4) | (src[7] >> 4))	& 31];
	dst[12] = BASE32_ALPHABET[((src[7] << 1) | (src[8] >> 7))	& 31];
	dst[13] = BASE32_ALPHABET[ (src[8] >> 2)			& 31];
	dst[14] = BASE32_ALPHABET[((src[8] << 3) | (src[9] >> 5))	& 31];
	dst[15] = BASE32_ALPHABET[  src[9]				& 31];

	dst[16] = '\0';
}

/////////////////////////////////////////////////////////////////
// Decode base32 16 character long 'src' into 10 byte long 'dst'                   // Thanks to Unperson Hiro for this routine

void base32_dec (BYTE *dst, char *src)
{
	char		tmp[BASE32_ONIONLEN];
	unsigned int	i;

	for (i = 0; i < 16; i++) {
		if (src[i] >= 'a' && src[i] <= 'z') {
			tmp[i] = src[i] - 'a';
		} else {
			if (src[i] >= '2' && src[i] <= '7')
				tmp[i] = src[i] - '1' + ('z' - 'a');
		 	else {
				/* Bad character detected.
				 * This should not happen, but just in case
				 * we will replace it with 'z' character. */
				tmp[i] = 0;
			}
		}
	}
	dst[0] = (tmp[ 0] << 3) | (tmp[1] >> 2);
	dst[1] = (tmp[ 1] << 6) | (tmp[2] << 1) | (tmp[3] >> 4);
	dst[2] = (tmp[ 3] << 4) | (tmp[4] >> 1);
	dst[3] = (tmp[ 4] << 7) | (tmp[5] << 2) | (tmp[6] >> 3);
	dst[4] = (tmp[ 6] << 5) |  tmp[7];
	dst[5] = (tmp[ 8] << 3) | (tmp[9] >> 2);
	dst[6] = (tmp[ 9] << 6) | (tmp[10] << 1) | (tmp[11] >> 4);
	dst[7] = (tmp[11] << 4) | (tmp[12] >> 1);
	dst[8] = (tmp[12] << 7) | (tmp[13] << 2) | (tmp[14] >> 3);
	dst[9] = (tmp[14] << 5) |  tmp[15];
}

///////////////////////////////////////////////////////////
// Print the projected time for a successful search

void PrintProjected(HWND hWnd)
{
    HWND    hWndPattern;
    int     i, len, iThreadCount;
    TCHAR   szThreads[3];
    DOUBLE  Odds, Projected, FullSpeed;

    hWndPattern = GetDlgItem (hWnd, IDC_PATTERN);
    len = GetWindowTextLength(hWndPattern);
    GetDlgItemText (hWnd, IDC_THREADS, szThreads, sizeof(szThreads) / sizeof (TCHAR));
    iThreadCount = atoi(szThreads);
    FullSpeed = iThreadCount * SpeedPerCore;
    Odds = 1;
    for (i=0; i<len; i++)
      {
        Odds = Odds * 32;
      } 
    Projected = Odds / FullSpeed;

    if (Projected < 0.01666666) strcpy(szBuffer, "< 1 second");
    if (Projected >= 0.01666666) sprintf(szBuffer, "% .2f  seconds", 60 * Projected);
    if (Projected >= 1) sprintf(szBuffer, "% .2f  minutes", Projected);
    if (Projected >= 60) sprintf(szBuffer, "% .2f  hours", Projected / 60);
    if (Projected >= 1440) sprintf(szBuffer, "% .2f  days", Projected / 1440);
    if (Projected >= 43830) sprintf(szBuffer, "% .2f  months", Projected / 43830);
    if (Projected >= 525960) sprintf(szBuffer, "% .2f  years", Projected / 525960);
    if (Projected >= 525960000) sprintf(szBuffer, "% .2f  millenia", Projected / 525960000);
    if (Projected >= 525960000000) sprintf(szBuffer, "% .2f  million years", Projected / 525960000000);
    if (Projected >= 525960000000000) sprintf(szBuffer, "% .2f  billion years", Projected / 525960000000000);
    if (len == 0) strcpy(szBuffer, "");
    SetDlgItemText(hWnd, IDC_TIME, szBuffer);
}

///////////////////////////////////////////////////////////
// Set initial directory of the browse for folder dialog

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
  switch(uMsg)
  {
    case BFFM_INITIALIZED:
      if (NULL != szCurDir)
      {
        SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)szCurDir);
      }
      break;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// WM_COMMAND handling.

BOOL MainDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl)
{
    int	len, i, j, k, iThreadCount;
    RSA     *rsa;
    BUF_MEM *buf1;
    BIO     *b;
    HWND    hWndPattern, hWndOkay, hWndAnimate, hWndRandom, hWndThreads;
    TCHAR   szThreads[3];
    HANDLE  hThread[99];

    switch (wCommand)
    {
    case IDC_RANDOM:
        SetDlgItemText(hWnd, IDC_TRIES, "");
        random (hWnd);
        hWndPattern=GetDlgItem (hWnd, IDC_PATTERN);                                // Place cursor in edit window so can type immediately
        SetFocus(hWndPattern) ;
        break;

    case IDC_PATTERN:
        switch (wNotify)
        {
        case EN_CHANGE:
            PrintProjected(hWnd);
            break;
        }
        break;

    case IDC_THREADS:
        switch (wNotify)
        {
        case EN_CHANGE:
            PrintProjected(hWnd);
            break;
        }
        break;

    case IDC_SAVE:
        LPMALLOC pMalloc;
        if(SUCCEEDED(SHGetMalloc(&pMalloc)))
        {
            BROWSEINFO bi = {0};
            bi.hwndOwner = hWnd;
            bi.lpszTitle = _T("Select folder to hold onion keys");
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            bi.lpfn = BrowseCallbackProc;

            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

            if(pidl)
            {
                SHGetPathFromIDList(pidl, szCurDir);
                pMalloc->Free(pidl);
                pMalloc->Release();
                sprintf(szFileTitle, "%s%s", szCurDir, "/hostname");
                FILE *file;
                if ((file = fopen (szFileTitle, "rt")))
                {
                    if (IDNO == (MessageBox(hWnd, "Overwrite existing keys?","Overwrite?",MB_YESNO | MB_ICONQUESTION)))
                    {
                        fclose (file);
                        break;
                    }
                fclose (file);
                }
                if (NULL == (file = fopen (szFileTitle, "wb"))) {break;}
                fprintf (file, "%s", szFullname) ;
                fclose (file);
                sprintf(szFileTitle, "%s%s", szCurDir, "/private_key");
                if (NULL == (file = fopen (szFileTitle, "wb")))
                {
                    MessageBox(hWnd, "Error writing file!","Error!",MB_OK);
                    break;
                }
                GetDlgItemText (hWnd, IDC_RSA_KEY, dst, sizeof(dst) / sizeof(char));
                fprintf (file, "%s", dst) ;
                fclose (file);
            }
        }
        break;

    case IDOK:
        if (params.bContinue) break;
        GetDlgItemText (hWnd, IDC_PATTERN, szPattern, sizeof(szPattern) / sizeof(TCHAR));
        hWndPattern=GetDlgItem (hWnd, IDC_PATTERN);
        len = GetWindowTextLength(hWndPattern);
        if (len == 0){break;}
        for (i = 0; i < len; i++)
        {
            if (!((szPattern[i] >= 'a' && szPattern[i] <= 'z') || (szPattern[i] >= '2' && szPattern[i] <= '7')))
            {
                MessageBox(hWnd, "There is a disallowed character in the submitted name", "Stop", MB_OK | MB_ICONSTOP);
                return FALSE;
            }
        }
        base32_dec (params.sha_pat, szPattern);                                    // determine SHA1 search pattern and pre-shift the last byte
        if (len == 1) {params.iSha = 0; params.iShift = 3; params.sha_pat[0] = params.sha_pat[0] >> 3;}
        if (len == 2) {params.iSha = 1; params.iShift = 6; params.sha_pat[1] = params.sha_pat[1] >> 6;}
        if (len == 3) {params.iSha = 1; params.iShift = 1; params.sha_pat[1] = params.sha_pat[1] >> 1;}
        if (len == 4) {params.iSha = 2; params.iShift = 4; params.sha_pat[2] = params.sha_pat[2] >> 4;}
        if (len == 5) {params.iSha = 3; params.iShift = 7; params.sha_pat[3] = params.sha_pat[3] >> 7;}
        if (len == 6) {params.iSha = 3; params.iShift = 2; params.sha_pat[3] = params.sha_pat[3] >> 2;}
        if (len == 7) {params.iSha = 4; params.iShift = 5; params.sha_pat[4] = params.sha_pat[4] >> 5;}
        if (len == 8) {params.iSha = 4; params.iShift = 0;}
        if (len == 9) {params.iSha = 5; params.iShift = 3; params.sha_pat[5] = params.sha_pat[5] >> 3;}
        if (len ==10) {params.iSha = 6; params.iShift = 6; params.sha_pat[6] = params.sha_pat[6] >> 6;}
        if (len ==11) {params.iSha = 6; params.iShift = 1; params.sha_pat[6] = params.sha_pat[6] >> 1;}
        if (len ==12) {params.iSha = 7; params.iShift = 4; params.sha_pat[7] = params.sha_pat[7] >> 4;}
        if (len ==13) {params.iSha = 8; params.iShift = 7; params.sha_pat[8] = params.sha_pat[8] >> 7;}
        if (len ==14) {params.iSha = 8; params.iShift = 2; params.sha_pat[8] = params.sha_pat[8] >> 2;}
        if (len ==15) {params.iSha = 9; params.iShift = 5; params.sha_pat[9] = params.sha_pat[9] >> 5;}
        if (len ==16) {params.iSha = 9; params.iShift = 0;}

        hWndOkay = GetDlgItem(hWnd, IDOK);
        SetFocus (hWndOkay);
        params.hWnd = hWnd;
        SetDlgItemText(hWnd, IDC_TRIES, "");
        SetDlgItemText(hWnd, IDC_ENTER_NAME, "Searching for");

	hWndAnimate=GetDlgItem (hWnd, IDC_ANIMATE);
	ShowWindow (hWndAnimate, SW_SHOW);
	hWndRandom=GetDlgItem (hWnd, IDC_RANDOM);
	ShowWindow (hWndRandom, SW_HIDE);

        hWndThreads = GetDlgItem(hWnd, IDC_THREADS);
        SendMessage(hWndThreads, EM_SETREADONLY, TRUE, NULL);

        GetDlgItemText (hWnd, IDC_THREADS, szThreads, sizeof(szThreads) / sizeof (TCHAR));
        iThreadCount=atoi(szThreads);
        params.iThreadCount=iThreadCount;

        for (i = 0; i < iThreadCount; i++)
        {
          hThread[i] = (HANDLE)_beginthread (thread, 0, &params);
          SetThreadPriority(hThread[i], THREAD_PRIORITY_IDLE);
        }
        tBegin = clock();
        SetTimer(hWnd, 1, 5000, NULL);
        SetDlgItemText(hWnd, IDC_TRIES, "Time elapsed: 0 seconds");
        break;

    case IDCANCEL:
        params.bContinue = FALSE;
        hWndAnimate=GetDlgItem (hWnd, IDC_ANIMATE);
        ShowWindow (hWndAnimate, SW_HIDE);
        hWndRandom=GetDlgItem (hWnd, IDC_RANDOM);
        ShowWindow (hWndRandom, SW_SHOW);
        hWndThreads = GetDlgItem(hWnd, IDC_THREADS);
        SendMessage(hWndThreads, EM_SETREADONLY, FALSE, NULL);
        KillTimer(hWnd, 1);
        break;

    case IDC_SETTINGS:
        DialogBox (hInst, "SettingsDlgBox", hWnd, SettingsDlgProc);
        break;

    case IDC_EXIT:
        PostMessage (hWnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
        break;

    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Main dialog message-handling function

BOOL CALLBACK MainDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
static NOTIFYICONDATA  nid;                                                        // For systray icon
       UINT            xCap, yCap;                                                 // Monitor's resolution
       HFONT           hFont;
       HWND            hWndPattern, hWndOnionName, hWndRSAKey, hWndAnimate;
       HDC             hDC;
       LPNMUPDOWN      lpnmud;
       int             i, len, left, top, iThreadCount;
       TCHAR           szThreads[3], buf[16], *pStr;
       DOUBLE          Odds, Projected, FullSpeed;
       clock_t         tEnd;
       double          elapsed;
       WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};

    switch (uMsg)
    {
    case WM_INITDIALOG:
        hDC = GetDC (hWnd);
        xCap = GetDeviceCaps (hDC, HORZRES);
        yCap = GetDeviceCaps (hDC, VERTRES) - BORDER_BAR;
        left=xCap/2-281;
        top=yCap/2-212;
        SetClassLong(hWnd, GCL_HICON, (LONG)LoadIcon(hInst, "garlic"));            // Add icon to title bar

        nid.cbSize = sizeof(NOTIFYICONDATA); 
        nid.hWnd = hWnd; 
        nid.uID = IDI_TRAYICON; 
        nid.uCallbackMessage = WM_TRAYICON; 
        nid.hIcon = LoadIcon(hInst, "garlic");
        strcpy(nid.szTip, "Garlic v0.0.5.1"); 
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
  
        hWndPattern   =GetDlgItem (hWnd, IDC_PATTERN)   ;                          // These three controls need a fixed width font
        hWndOnionName =GetDlgItem (hWnd, IDC_ONION_NAME);
        hWndRSAKey    =GetDlgItem (hWnd, IDC_RSA_KEY)   ;
        hFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
        SendMessage (hWndPattern,    WM_SETFONT, (WPARAM) hFont,0);
        SendMessage (hWndOnionName,  WM_SETFONT, (WPARAM) hFont,0);
        SendMessage (hWndRSAKey,     WM_SETFONT, (WPARAM) hFont,0);
        SetDlgItemText(hWnd, IDC_THREADS, "1");
        hWndAnimate=GetDlgItem (hWnd, IDC_ANIMATE);                                // For animation while seeking a pattern match
        ShowWindow  (hWndAnimate,    SW_HIDE);                                     // Don't show animation at startup
        GetCurrentDirectory((DWORD)_MAX_PATH, szCurDir);                           // Start folder browsing here
        SendMessage (hWnd, WM_COMMAND, IDC_RANDOM, 0);                             // Launch with a random key visible

        GetCurrentDirectory(_MAX_PATH, szLogFilePath);
        lstrcat(szLogFilePath, "\\logs\\");
        GetModuleFileName(hInst, szIniFilePath, _MAX_PATH);                        // Prepare to load ini file
        pStr=strrchr(szIniFilePath, '.');
        if (pStr != NULL) *(++pStr)='\0';
        lstrcat(szIniFilePath, "ini");
        GetPrivateProfileString("Settings", "Benchmarked speed", "!", buf, 16, szIniFilePath);

        if (buf[0] == '!')                                                         // First run, benchmark, then create ini file
        {
          benchmark(hWnd);
          _itoa(SpeedPerCore, szBuffer, 10);
          WritePrivateProfileString("Settings", "Remember benchmark", "1", szIniFilePath);
          WritePrivateProfileString("Settings", "Benchmarked speed", szBuffer, szIniFilePath);
          WritePrivateProfileString("Settings", "Remember # of threads", "1", szIniFilePath);
          WritePrivateProfileString("Settings", "Number of threads", "1", szIniFilePath);
          WritePrivateProfileString("Settings", "Remember search term", "1", szIniFilePath);
          WritePrivateProfileString("Settings", "Last search term", "", szIniFilePath);
          WritePrivateProfileString("Settings", "Remember last path", "1", szIniFilePath);
          WritePrivateProfileString("Settings", "Last path", szCurDir, szIniFilePath);
          WritePrivateProfileString("Settings", "Recall window position", "0", szIniFilePath);
          WritePrivateProfileString("Settings", "Window position", "", szIniFilePath);
          WritePrivateProfileString("Settings", "Logging", "1", szIniFilePath);
        }
        else                                                                       // An ini file exists so use it
        {
          if (buf[0] != '\0')
          {
            SpeedPerCore=atoi(buf);
          }
          if (buf[0] == '\0')
          {
            benchmark(hWnd);
          }
          GetPrivateProfileString("Settings", "Remember # of threads", "1", buf, 16, szIniFilePath);
          if (buf[0] == '1')
          {
              GetPrivateProfileString("Settings", "Number of threads", "1", buf, 16, szIniFilePath);
              SetDlgItemText(hWnd, IDC_THREADS, buf);
          }
          GetPrivateProfileString("Settings", "Remember search term", "1", buf, 16, szIniFilePath);
          if (buf[0] == '1')
          {
              GetPrivateProfileString("Settings", "Last search term", "", buf, 16, szIniFilePath);
              SetDlgItemText(hWnd, IDC_PATTERN, buf);
          }
          GetPrivateProfileString("Settings", "Remember last path", "1", buf, 16, szIniFilePath);
          if (buf[0] == '1')
          {
              GetPrivateProfileString("Settings", "Last path", "", szCurDir, _MAX_PATH, szIniFilePath);
          }
          GetPrivateProfileString("Settings", "Recall window position", "0", buf, 16, szIniFilePath);
          if (buf[0] == '1')
          {
              GetPrivateProfileString("Settings", "Window position", "", buf, 16, szIniFilePath);
              sscanf(buf, "%d,%d", &left, &top);
          }
        }
        MoveWindow (hWnd, left, top, 561, 423, TRUE);
        sprintf(szBuffer, "                          Benchmarked tries per minute: %.2f million", SpeedPerCore/1000000);
        SetDlgItemText(hWnd, IDC_TRIES, szBuffer);
        return TRUE;

    case WM_TRAYICON:
      switch (lParam)
      {
        case WM_LBUTTONUP:
        {
            Shell_NotifyIcon(NIM_DELETE, &nid);                                    // Bring window back to the foreground
            ShowWindow(hWnd, SW_SHOWNORMAL);
            SetForegroundWindow (hWnd);
            return TRUE;

        }
        case WM_RBUTTONUP:
        {
            POINT pt;
            HMENU hMenu = NULL;
            hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, IDM_TRAYRESTORE, "Restore Garlic");
            AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);
            AppendMenu(hMenu, MF_STRING, IDM_TRAYEXIT, "Exit Garlic");
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            UINT clicked = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
            if (clicked == IDM_TRAYRESTORE)                                        // Restore window
            {
                Shell_NotifyIcon(NIM_DELETE, &nid);
                ShowWindow(hWnd, SW_SHOWNORMAL);
                SetForegroundWindow (hWnd);
            }
            if (clicked == IDM_TRAYEXIT) SendMessage(hWnd, WM_CLOSE, 0, 0);
            DestroyMenu(hMenu);
            return TRUE;
        }
      }

    case WM_SIZE:
      switch (wParam)
      {
        case SIZE_MINIMIZED:
        {
          if (params.bContinue)
          {
            strcpy(szBuffer, "Garlic v0.0.5.1\nSeeking ");
            strcat(szBuffer, szPattern);
            strcpy(nid.szTip, szBuffer);
          }
          if (!params.bContinue) strcpy(nid.szTip, "Garlic v0.0.5.1\nIdle");
          Shell_NotifyIcon(NIM_ADD, &nid);
          ShowWindow (hWnd, SW_HIDE);
          return TRUE;
        }
      }

    case WM_COMMAND:
        return MainDialog_OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);

    case WM_CLOSE:
        GetPrivateProfileString("Settings", "Remember benchmark", "1", buf, 16, szIniFilePath);
        if (buf[0] == '0')
        {
            WritePrivateProfileString("Settings", "Benchmarked speed", "", szIniFilePath);
        }
        else
        {
            _itoa(SpeedPerCore, szBuffer, 10);
            WritePrivateProfileString("Settings", "Benchmarked speed", szBuffer, szIniFilePath);
        }
        GetPrivateProfileString("Settings", "Remember # of threads", "1", buf, 16, szIniFilePath);
        if (buf[0] == '0')
        {
            WritePrivateProfileString("Settings", "Number of threads", "1", szIniFilePath);
        }
        else
        {
            GetDlgItemText (hWnd, IDC_THREADS, szThreads, sizeof(szThreads) / sizeof (TCHAR));
            WritePrivateProfileString("Settings", "Number of threads", szThreads, szIniFilePath);
        }
        GetPrivateProfileString("Settings", "Remember search term", "1", buf, 16, szIniFilePath);
        if (buf[0] == '0')
        {
            WritePrivateProfileString("Settings", "Last search term", "", szIniFilePath);
        }
        else
        {
            GetDlgItemText (hWnd, IDC_PATTERN, szPattern, sizeof(szPattern) / sizeof(TCHAR));
            WritePrivateProfileString("Settings", "Last search term", szPattern, szIniFilePath);
        }
        GetPrivateProfileString("Settings", "Remember last path", "1", buf, 16, szIniFilePath);
        if (buf[0] == '0')
        {
            WritePrivateProfileString("Settings", "Last path", "", szIniFilePath);
        }
        else
        {
            WritePrivateProfileString("Settings", "Last path", szCurDir, szIniFilePath);
        }
        GetPrivateProfileString("Settings", "Recall window position", "0", buf, 16, szIniFilePath);
        if (buf[0] == '0')
        {
            WritePrivateProfileString("Settings", "Window position", "", szIniFilePath);
        }
        else
        {
            GetWindowPlacement (hWnd, &wp);
            sprintf(buf, "%d,%d", wp.rcNormalPosition.left, wp.rcNormalPosition.top);
            WritePrivateProfileString("Settings", "Window position", buf, szIniFilePath);
        }

        /* Now let's clean up after ourselves */
        Shell_NotifyIcon(NIM_DELETE, &nid);
        KillTimer(hWnd, 1);
        EndDialog(hWnd, 0);
        return TRUE;

    case WM_NOTIFY:
        switch (LOWORD(wParam))
        {
            case IDC_UPDOWN:                                                       // Spinner
                if (!params.bContinue)
                {
                lpnmud = (LPNMUPDOWN) lParam;
                GetDlgItemText (hWnd, IDC_THREADS, szThreads, sizeof(szThreads) / sizeof (TCHAR));
                i = atoi (szThreads);
                if (lpnmud->iDelta<0)
                {
                    i+=1;
                    if (i > 64) i = 1;
                }
                else
                {
                    i-=1;
                    if (i > 64) i = 64;
                    if (i < 1) i = 64;
                }
                sprintf(szThreads, "%d", i);
                SetDlgItemText(hWnd, IDC_THREADS, szThreads);
                PrintProjected(hWnd);
                }
                return TRUE;
        }

    case WM_TIMER:
            tEnd = clock();
            elapsed = difftime(tEnd, tBegin);
            if (elapsed < 60000) sprintf(szBuffer, "Time elapsed: %.0f seconds", elapsed/1000);
            if (elapsed >= 60000) sprintf(szBuffer, "Time elapsed: %.2f minutes", elapsed/60000);
            if (elapsed >= 3600000) sprintf(szBuffer, "Time elapsed: %.2f hours", elapsed/3600000);
            if (elapsed >= 86400000) sprintf(szBuffer, "Time elapsed: %.2f days", elapsed/86400000);
            SetDlgItemText(hWnd, IDC_TRIES, szBuffer);
            return TRUE;

    default:
        if (uMsg == TASKBARCREATED)
          {
            Shell_NotifyIcon(NIM_ADD, &nid);
            return TRUE;
          }
    }
    return FALSE;
}

BOOL CALLBACK SettingsDlgProc (HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    int     i;
    TCHAR   buf[16];

    switch (iMsg)
    {
        case WM_INITDIALOG:
            GetPrivateProfileString("Settings", "Remember benchmark", "1", buf, 16, szIniFilePath);
            if (buf[0] == '1')
            {
                CheckDlgButton(hWnd, ID_BENCHMARK, BST_CHECKED);
            }
            GetPrivateProfileString("Settings", "Remember # of threads", "1", buf, 16, szIniFilePath);
            if (buf[0] == '1')
            {
                CheckDlgButton(hWnd, ID_THREADS, BST_CHECKED);
            }
            GetPrivateProfileString("Settings", "Remember search term", "1", buf, 16, szIniFilePath);
            if (buf[0] == '1')
            {
                CheckDlgButton(hWnd, ID_SEARCH, BST_CHECKED);
            }
            GetPrivateProfileString("Settings", "Remember last path", "1", buf, 16, szIniFilePath);
            if (buf[0] == '1')
            {
                CheckDlgButton(hWnd, ID_PATH, BST_CHECKED);
            }
            GetPrivateProfileString("Settings", "Recall window position", "0", buf, 16, szIniFilePath);
            if (buf[0] == '1')
            {
                CheckDlgButton(hWnd, ID_POSITION, BST_CHECKED);
            }
            GetPrivateProfileString("Settings", "Logging", "1", buf, 16, szIniFilePath);
            if (buf[0] == '0')
            {
                CheckDlgButton(hWnd, ID_LOG0, BST_CHECKED);
            }
            if (buf[0] == '1')
            {
                CheckDlgButton(hWnd, ID_LOG1, BST_CHECKED);
            }
            if (buf[0] == '2')
            {
                CheckDlgButton(hWnd, ID_LOG2, BST_CHECKED);
            }
            return TRUE;

        case WM_COMMAND:

            switch (LOWORD(wParam))
            {
                case ID_BENCHMARK:
                    if (SendDlgItemMessage(hWnd, ID_BENCHMARK, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                    {
                        WritePrivateProfileString("Settings", "Remember benchmark", "0", szIniFilePath);
                    }
                    else
                    {
                        WritePrivateProfileString("Settings", "Remember benchmark", "1", szIniFilePath);
                    }
                    return TRUE;

                case ID_THREADS:
                    if (SendDlgItemMessage(hWnd, ID_THREADS, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                    {
                        WritePrivateProfileString("Settings", "Remember # of threads", "0", szIniFilePath);
                    }
                    else
                    {
                        WritePrivateProfileString("Settings", "Remember # of threads", "1", szIniFilePath);
                    }
                    return TRUE;

                case ID_SEARCH:
                    if (SendDlgItemMessage(hWnd, ID_SEARCH, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                    {
                        WritePrivateProfileString("Settings", "Remember search term", "0", szIniFilePath);
                    }
                    else
                    {
                        WritePrivateProfileString("Settings", "Remember search term", "1", szIniFilePath);
                    }
                    return TRUE;

                case ID_PATH:
                    if (SendDlgItemMessage(hWnd, ID_PATH, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                    {
                        WritePrivateProfileString("Settings", "Remember last path", "0", szIniFilePath);
                    }
                    else
                    {
                        WritePrivateProfileString("Settings", "Remember last path", "1", szIniFilePath);
                    }
                    return TRUE;

                case ID_POSITION:
                    if (SendDlgItemMessage(hWnd, ID_POSITION, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                    {
                        WritePrivateProfileString("Settings", "Recall window position", "0", szIniFilePath);
                    }
                    else
                    {
                        WritePrivateProfileString("Settings", "Recall window position", "1", szIniFilePath);
                    }
                    return TRUE;

                case ID_LOG0:
                    WritePrivateProfileString("Settings", "Logging", "0", szIniFilePath);
                    return TRUE;

                case ID_LOG1:
                    WritePrivateProfileString("Settings", "Logging", "1", szIniFilePath);
                    return TRUE;

                case ID_LOG2:
                    WritePrivateProfileString("Settings", "Logging", "2", szIniFilePath);
                    return TRUE;

                case IDOK:
                    EndDialog (hWnd, TRUE);
                    return TRUE;

                case IDCANCEL:
                    EndDialog (hWnd, FALSE);
                    return TRUE;
            }
    }
    return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) 
{
    hInst = hInstance;
    InitCommonControls();
    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)MainDialogProc, 0);
    return 0;
}
