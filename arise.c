/*
  Copyright (c) 2005-2018 Chris Cuthbertson

  This file is part of arise.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "arise.h"

popup_t *popups;
CRITICAL_SECTION *cs;
WNDCLASS *wc;
HINSTANCE hinst;

#ifdef LEGACY_WIN9X
HMODULE usr;
PSLWA pSetLayeredWindowAttributes;
ANIWIN pAnimateWindow;
#endif

#ifdef _VERBOSE
HWND hdebug = NULL;
HWND hedit = NULL;
HANDLE thdebug = NULL;
int thwait = 0;
#endif

int WINAPI _DllMainCRTStartup(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved)
{
  switch(fdwReason)
  {
    case DLL_PROCESS_ATTACH:
      Init(hInstDll);
      break;

    case DLL_PROCESS_DETACH:
      Finish();

#ifdef _VERBOSE
      if (thdebug)
      {
        if (hdebug)
          SendMessage(hdebug, WM_CLOSE, (WPARAM)0, (LPARAM)0);
        WaitForSingleObject(thdebug, INFINITE);
      }
#endif
      break;

    case DLL_THREAD_ATTACH:
      return 0;

    case DLL_THREAD_DETACH:
      return 0;
  }

  return 1;
}

void __stdcall LoadDll(LOADINFO *li)
{
  li->mKeep = TRUE;
}

int __stdcall UnloadDll(int timeout)
{
  return 1;
}

int __stdcall ShowPopup(HWND mWnd, HWND aWnd, char *data, char *parms, BOOL show, BOOL nopause)
{
  int r;

#ifdef _VERBOSE
  yay("ShowPopup(): New Popup: \"%s\" \"%s\"", data, parms);
#endif

#ifdef LEGACY_SUPPORT
  r = ParseDetect(data);
#else
  r = ParsePopupV2(data);
#endif

#ifdef _VERBOSE
  yay("ShowPopup(): Errors returned: %d", r);
#endif

  if (r != ARISE_NO_ERROR)
  {
#ifdef _VERBOSE
  yay("ShowPopup(): Error ....");
#endif

    lstrcpy(data, "echo -ts Popup error: ");

    switch (r)
    {
      case ARISE_OUT_OF_MEMORY:
        lstrcat(data, "no memory");
        break;

      case ARISE_BAD_DELIMITERS:
        lstrcat(data, "bad delimiters");
        break;

      case ARISE_MISSING_TEXT:
        lstrcat(data, "missing text");
        break;

      case ARISE_MASSIVE_FAILURE:
        lstrcat(data, "fatal");
        break;

      default:
#ifdef _VERBOSE
        yay("ShowPopup(): UMM missing error code");
        lstrcat(data, "unknown");
#endif
        break;
    }

    return MIRC_COMMAND;
  }

#ifdef _VERBOSE
  yay("ShowPopup(): Finished OK");
#endif

  return MIRC_CONTINUE;
}

int __stdcall Version(HWND mWnd, HWND aWnd, char *data, char *parms, BOOL show, BOOL nopause)
{
  int b;

  b = (*data && !nopause) ? 1 : 0;
  lstrcpy(data, HEADER_NAME " " HEADER_VER HEADER_VEREX);
  lstrcat(data, (b) ? "\r\n" : " - ");
  lstrcat(data, HEADER_COPYRIGHT " " HEADER_AUTHOR " (" HEADER_EMAIL ") ");
  lstrcat(data, (b) ? "\r\n" : " - ");
  lstrcat(data, HEADER_WEBSITE);
  if (b)
  {
    MessageBox(mWnd, data, HEADER_NAME, MB_OK | MB_ICONINFORMATION);
    return MIRC_CONTINUE;
  }

  return MIRC_IDENTIFIER;
}

#ifdef _VERBOSE
int __stdcall Debug(HWND mWnd, HWND aWnd, char *data, char *parms, BOOL show, BOOL nopause)
{
  int i;
  popup_t *p;

  if (!thdebug)
  {
    thdebug = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InfoWindow, NULL, 0, &i);
    if (!thdebug)
    {
      MessageBox(NULL, "DebugWindow() failed", "gah", MB_OK | MB_ICONEXCLAMATION);
    }
  }
  else
  {
    if (popups)
    {
      yay("Current active popups:");
      for (i = 0, p = popups; p; p = p->next, i++)
        yay("%02d: %s", i, p->name);
      yay("total: %d", i);
    }
    else
    {
      yay("No active popups");
    }
  }

  if (thwait)
    WaitForSingleObject(thdebug, INFINITE);

  return MIRC_CONTINUE;
}
#endif

#ifdef _VERBOSE
int __stdcall ThreadWait(int wait)
{
  thwait = wait;
  yay("ThreadWait() Changed to: %d", thwait);

  return 0;
}
#endif

int __stdcall Arise(const char *tfont, int theight, int tstyle, const char *tcolour,
                    const char *bfont, int bheight, int bstyle, const char *bcolour,
                    const char *mcolour, int mtime, int mstyle, int mpoint,
                    int malpha, int mshowani, int mshowtime, int mhideani, int mhidetime,
                    const char *ttext, const char *btext)
{
  popup_t *popup;
  int len, r;

#ifdef _VERBOSE
  yay("Arise()");
#endif

  if ((!ttext || !*ttext) && (!btext || !*btext))
    return ARISE_MISSING_TEXT;

  if ((r = Begin()) != ARISE_NO_ERROR)
    return r;

  if ((r = AddPopup(&popup)) != ARISE_NO_ERROR)
  {
    RemovePopup(popup);
    return r;
  }

  if (((len = lstrlen(tfont)) > 0) && (popup->title->fontname = (char *)ALLOC_MEM(len + 1)))
    lstrcpy(popup->title->fontname, tfont);
  popup->title->fontheight = theight;
  popup->title->fontstyle = tstyle;
  popup->title->fontcolour = readrgb(tcolour);
  if (((len = lstrlen(bfont)) > 0) && (popup->body->fontname = (char *)ALLOC_MEM(len + 1)))
    lstrcpy(popup->body->fontname, bfont);
  popup->body->fontheight = bheight;
  popup->body->fontstyle = bstyle;
  popup->body->fontcolour = readrgb(bcolour);
  if (!(popup->bgcolour = CreateSolidBrush(readrgb(mcolour))))
    popup->bgcolour = (HBRUSH)GetStockObject(WHITE_BRUSH);
  popup->time = clamp(50, TIMER_MAX, mtime);
  popup->alpha = malpha;
  popup->winop = mstyle;
  popup->point = mpoint;
  popup->showani = aniflag(mshowani);
  popup->showtime = mshowtime;
  popup->hideani = aniflag(mhideani);
  popup->hidetime = mhidetime;
  if (((len = lstrlen(ttext)) > 0) && (popup->title->text = (char *)ALLOC_MEM(len + 1)))
    lstrcpy(popup->title->text, ttext);
  if (((len = lstrlen(btext)) > 0) && (popup->body->text = (char *)ALLOC_MEM(len + 1)))
    lstrcpy(popup->body->text, btext);

#ifdef _VERBOSE

  yay("Arise() creating thread");
#endif
  popup->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CreatePopup, (LPVOID)&*popup, 0, &r);
  if (!popup->thread)
    return ARISE_MASSIVE_FAILURE;

#ifdef _VERBOSE
  if (thwait)
    WaitForSingleObject(popup->thread, INFINITE);
#endif

#ifdef _VERBOSE
  yay("Arise() finished OK");
#endif

  return ARISE_NO_ERROR;
}

#ifdef LEGACY_SUPPORT
int ParseDetect(char *data)
{
#ifdef LEGACY_VERSION_1
  int i = 0;
  char *p;

#ifdef _VERBOSE
  yay("ParseDetect() %s", data);
#endif

  p = data;
  while (*p)
  {
    if (*p++ == POPUP_V1_DELIMITER)
      i++;
    if (i == (POPUP_V1_NUM_DELIMITERS - 1))
      return ParsePopupV1(data);
  }
#endif

  return ParsePopupV2(data);
}
#endif

#ifdef LEGACY_VERSION_1
int ParsePopupV1(char *data)
{
  char *p[POPUP_V1_NUM_DELIMITERS];
  char *s;
  int i;

  if (!(s = data))
    return ARISE_MISSING_TEXT;

  for (i = 0; i < POPUP_V1_NUM_DELIMITERS; i++)
  {
    p[i] = s;

    while (*s)
    {
      if (*s == POPUP_V1_DELIMITER)
      {
        *s++ = '\0';
        break;
      }
      s++;
    }
  }

  if (i != POPUP_V1_NUM_DELIMITERS)
  {
#ifdef _VERBOSE
    yay("ParsePopupV1() OOPS: bad delimiters: %s", data);
#endif
    return ARISE_BAD_DELIMITERS;
  }

#ifdef _VERBOSE
  //for (i = 0; i < POPUP_NUM_DELIMITERS; i++)
  //  yay("ParsePopup() param: %s", p[i]);
#endif

  i = Arise(p[PARSE_V1_TITLE_FONTNAME],
            _atoi(p[PARSE_V1_TITLE_FONTHEIGHT]),
            _atoi(p[PARSE_V1_TITLE_FONTSTYLE]),
            p[PARSE_V1_TITLE_FONTCOLOUR],
            p[PARSE_V1_TITLE_FONTNAME],
            _atoi(p[PARSE_V1_BODY_FONTHEIGHT]),
            _atoi(p[PARSE_V1_BODY_FONTSTYLE]),
            p[PARSE_V1_BODY_FONTCOLOUR],
            p[PARSE_V1_BACKGROUND_COLOUR],
            _atoi(p[PARSE_V1_SHOW_TIME]),
            EFFECT_BORDER,
            0,
            0,
            0,
            0,
            0,
            0,
            p[PARSE_V1_TITLE_TEXT],
            p[PARSE_V1_BODY_TEXT]
           );

#ifdef _VERBOSE
  yay("ParsePopupV1() finished: %d", i);
#endif

  return i;
}
#endif

int ParsePopupV2(char *data)
{
  char *p[POPUP_V2_NUM_DELIMITERS];
  char *s, *c;
  int i, esc;

  if (!(s = data))
    return ARISE_MISSING_TEXT;

  for (i = 0, s = data, esc = 0; i < POPUP_V2_NUM_DELIMITERS; i++)
  {
    p[i] = s;
    c = s;

    if (!*s)
      break;

    while (*s)
    {
      if (*s == POPUP_V2_ESCAPE)
      {
        if (!esc)
        {
          s++;
          esc = 1;
          continue;
        }
      }

      if (*s == POPUP_V2_DELIMITER && !esc)
      {
        *s++ = '\0';
        break;
      }

      esc = 0;
      *c++ = *s++;
    }

    *c = '\0';
  }

  if (i != POPUP_V2_NUM_DELIMITERS)
  {
#ifdef _VERBOSE
    yay("ParsePopupV2() OOPS: bad delimiters: %s", data);
#endif
    return ARISE_BAD_DELIMITERS;
  }

#ifdef _VERBOSE
  //for (i = 0; i < POPUP_NUM_DELIMITERS; i++)
  //  yay("ParsePopup() param: %s", p[i]);
#endif

  i = Arise(p[PARSE_V2_TITLE_FONTNAME],
            _atoi(p[PARSE_V2_TITLE_FONTHEIGHT]),
            _atoi(p[PARSE_V2_TITLE_FONTSTYLE]),
            p[PARSE_V2_TITLE_FONTCOLOUR],
            p[PARSE_V2_BODY_FONTNAME],
            _atoi(p[PARSE_V2_BODY_FONTHEIGHT]),
            _atoi(p[PARSE_V2_BODY_FONTSTYLE]),
            p[PARSE_V2_BODY_FONTCOLOUR],
            p[PARSE_V2_BACKGROUND_COLOUR],
            clamp(50, TIMER_MAX, _atoi(p[PARSE_V2_SHOW_TIME])),
            _atoi(p[PARSE_V2_WINDOW_OPTION]),
            _atoi(p[PARSE_V2_WINDOW_POSITION]),
            _atoi(p[PARSE_V2_TRANSPARENCY]),
            _atoi(p[PARSE_V2_ANIMATION_SHOW_EFFECTS]),
            _atoi(p[PARSE_V2_ANIMATION_SHOW_TIME]),
            _atoi(p[PARSE_V2_ANIMATION_HIDE_EFFECTS]),
            _atoi(p[PARSE_V2_ANIMATION_HIDE_TIME]),
            p[PARSE_V2_TITLE_TEXT],
            p[PARSE_V2_BODY_TEXT]
           );

#ifdef _VERBOSE
  yay("ParsePopupV2() finished: %d", i);
#endif

  return i;
}

void *CreatePopup(LPVOID *data)
{
  popup_t *popup;
  MSG msg;
  HDC hdc;

  popup = (popup_t *)data;

#ifdef _VERBOSE
  strlcpy(popup->name, ((popup->body->text) ? popup->body->text : ""), sizeof(popup->name));
  yay("CreatePopup() started %s", popup->name);
#endif

  popup->hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | ((popup->alpha > 0) ? WS_EX_LAYERED : 0), POPUP_CLASS, "",
    WS_POPUP | DS_SETFONT | DS_FIXEDSYS | ((popup->winop & EFFECT_BORDER) ? WS_BORDER : 0), 0, 0, 1, 1, NULL, NULL, hinst, 0);
  if (!popup->hwnd)
  {
#ifdef _VERBOSE
    yay("CreateWindowEx() failed: %s", popup->name);
#endif
    goto _err;
  }

#ifdef _VERBOSE
  yay("CreatePopup() WindowLong: %s", popup->name); //v
#endif
  SetWindowLong(popup->hwnd, GWL_USERDATA, (long)popup);

#ifdef _VERBOSE
  yay("CreatePopup() Font Height: %s", popup->name); //v
#endif
  hdc = GetDC(popup->hwnd);
  popup->title->fontheight = -MulDiv(popup->title->fontheight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  popup->body->fontheight = -MulDiv(popup->body->fontheight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  ReleaseDC(popup->hwnd, hdc);

#ifdef _VERBOSE
  yay("CreatePopup() Font: %s", popup->name); //v
#endif

  if (popup->title->fontname)
  {
    popup->title->font = CreateFont(popup->title->fontheight, 0, 0, 0,
      (popup->title->fontstyle & FONT_STYLE_BOLD) ? 700 : 400,
      (popup->title->fontstyle & FONT_STYLE_ITALIC) ? 1 : 0,
      (popup->title->fontstyle & FONT_STYLE_UNDERLINE) ? 1 : 0, 0,
      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, popup->title->fontname);
  }
  if (!popup->title->font)
    popup->title->font = GetStockObject(SYSTEM_FONT);

  if (popup->body->fontname)
  {
    popup->body->font = CreateFont(popup->body->fontheight, 0, 0, 0,
      (popup->body->fontstyle & FONT_STYLE_BOLD) ? 700 : 400,
      (popup->body->fontstyle & FONT_STYLE_ITALIC) ? 1 : 0,
      (popup->body->fontstyle & FONT_STYLE_UNDERLINE) ? 1 : 0, 0,
      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, popup->body->fontname);
  }
  if (!popup->body->font)
    popup->body->font = GetStockObject(SYSTEM_FONT);

#ifdef _VERBOSE
  yay("CreatePopup() aplha: %d", popup->alpha); //v
#endif

  if (popup->alpha > 0)
  {
    //blend only works on non-layered windows
    UsrSetLayeredWindowAttributes(popup->hwnd, 0, (unsigned char)popup->alpha, LWA_ALPHA);
  }

  if (popup->showani && popup->showtime)
  {
    UsrAnimateWindow(popup->hwnd, popup->showtime, popup->showani);
  }

#ifdef _VERBOSE
  yay("CreatePopup() Set border: %s", popup->name); //v
#endif
   //SetWindowLong(popup->hwnd, GWL_STYLE, GetWindowLong(popup->hwnd, GWL_STYLE) | WS_BORDER);

#ifdef _VERBOSE
  yay("CreatePopup() Timer: %s", popup->name); //v
#endif
  SetTimer(popup->hwnd, TIMER_ID, popup->time, NULL);

#ifdef _VERBOSE
  yay("CreatePopup() ShowWin: %s", popup->name); //v
#endif
  ShowWindow(popup->hwnd, SW_SHOWNOACTIVATE);

#ifdef _VERBOSE
  yay("CreatePopup() GetMessage: %s", popup->name); //v
#endif
  while (GetMessage(&msg, popup->hwnd, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

#ifdef _VERBOSE
  yay("CreatePopup() finished OK: %s", popup->name);
#endif

_err:
  RemovePopup(popup);

  return 0;
}

void DestroyPopup(HWND hwnd)
{
  popup_t *popup;

  popup = (popup_t *)GetWindowLong(hwnd, GWL_USERDATA);
  if (!popup)
    return;

#ifdef _VERBOSE
  yay("DestroyPopup() %s", popup->name);
#endif

  if (popup->hideani && popup->hidetime)
  {
#ifdef _VERBOSE
    yay("DestroyPopup() hide animation %s", popup->name);
#endif
    UsrAnimateWindow(hwnd, popup->hidetime, popup->hideani | AW_HIDE);
  }

  DestroyWindow(hwnd);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uiMsg)
  {
    case WM_RBUTTONDOWN:
      KillTimer(hwnd, TIMER_ID);
      break;

    case WM_TIMER:
    case WM_CLOSE: //heh, don't use WM_DESTROY here or win98 DIES
      KillTimer(hwnd, TIMER_ID);
      DestroyPopup(hwnd);
      break;

    case WM_PAINT:
      Paint(hwnd);
      break;

    //case WM_ERASEBKGND:
    //  return 1;

    case WM_LBUTTONDOWN:
      SetTimer(hwnd, TIMER_ID, 0, NULL);
      break;

    case WM_PRINTCLIENT:
      PrintClient(hwnd, (HDC)wParam);
      return 0;

    default:
      break;
  }

  return DefWindowProc(hwnd, uiMsg, wParam, lParam);
}

void PrintClient(HWND hwnd, HDC hdc)
{
  PAINTSTRUCT ps;

#ifdef _VERBOSE
  yay("PrintClient()");
#endif

  ps.hdc = hdc;
  //GetClientRect(hwnd, &ps.rcPaint);
  PaintPopup(hwnd, &ps);
}

void Paint(HWND hwnd)
{
  PAINTSTRUCT ps;

#ifdef _VERBOSE
  yay("Paint()");
#endif

  BeginPaint(hwnd, &ps);
  PaintPopup(hwnd, &ps);
  EndPaint(hwnd, &ps);
}

//void PaintPopup(HWND hwnd)
void PaintPopup(HWND hwnd, PAINTSTRUCT *pps)
{
  popup_t *popup;
  RECT rc;
  int x, y;
  int width, height;

  popup = (popup_t *)GetWindowLong(hwnd, GWL_USERDATA);
  if (!popup)
  {
#ifdef _VERBOSE
  yay("PaintPopup() OOPS, missed data");
#endif
    return;
  }

  SetMapMode(pps->hdc, MM_TEXT);
  SetBkMode(pps->hdc, TRANSPARENT);

  if (!popup->repaint)
  {
    width = CalcTextWidth(popup, &pps->hdc) + (POPUP_BORDER * 2) + (POPUP_PADDING * 2);
    height = CalcTextHeight(popup, width, &popup->repaint, &pps->hdc) + (POPUP_BORDER * 2) + (POPUP_PADDING * 2);

    if (popup->point)
    {
      POINT pt;
      HMONITOR hMon;
      MONITORINFO mi;

      pt.x = popup->point;
      pt.y = 0;

      hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
      if (!hMon)
        goto _def;
      mi.cbSize = sizeof(MONITORINFO);
      if (!GetMonitorInfo(hMon, &mi))
        goto _def;
      rectcpy(&rc, &mi.rcWork);
    }
    else
    {
_def:
      SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
    }

    if (popup->next) //start from other popup
    {
      EnterCriticalSection(cs);
      if (popup->winop & EFFECT_POPUPTOP)
        y = popup->next->pos.bottom + POPUP_SPACE;
      else //EFFECT_POPUPBOTTOM
        y = popup->next->pos.top - height - POPUP_SPACE;
      LeaveCriticalSection(cs);

      if (popup->winop & EFFECT_POPUPLEFT)
        x = rc.left + POPUP_SPACE;
      else //EFFECT_POPUPRIGHT
        x = rc.right - width - POPUP_SPACE;

#ifdef _VERBOSE
      yay("PaintPopup() Based on previous, co-ords: %d,%d: %s", x, y, popup->name);
#endif
    }

#ifdef _VERBOSE
    yay("PaintPopup() %dx%d: %s", (rc.right - rc.left), (rc.bottom - rc.top), popup->name);
#endif

    if (!popup->next || (y < 0) || ((y + height) > (rc.bottom - rc.top))) //first popup or existing and off screen
    {
      if (popup->winop & EFFECT_POPUPLEFT)
        x = rc.left + POPUP_SPACE;
      else //EFFECT_POPUPRIGHT
        x = rc.right - width - POPUP_SPACE;

      if (popup->winop & EFFECT_POPUPTOP)
        y = rc.top + POPUP_SPACE;
      else //EFFECT_POPUPBOTTOM
        y = rc.bottom - height - POPUP_SPACE;

#ifdef _VERBOSE
      yay("PaintPopup() First popup, co-ords: %d,%d: %s", x, y, popup->name);
#endif
    }

    SetWindowPos(popup->hwnd, NULL, x, y, width, height, SWP_NOACTIVATE);

    GetWindowRect(popup->hwnd, &rc);
    rectcpy(&popup->pos, &rc);
  }
#ifdef _VERBOSE
  yay("PaintPopup() Pos: %d,%d,%d,%d %dx%d: %s", popup->pos.top, popup->pos.left, popup->pos.right, popup->pos.bottom, width, height, popup->name);
#endif

  GetClientRect(popup->hwnd, &rc);
  FillRect(pps->hdc, &rc, popup->bgcolour);
  PrintText(popup, popup->repaint, &rc, &pps->hdc);

#ifdef _VERBOSE
  yay("PaintPopup() End: %s", popup->name); //v
#endif
}

int CalcTextWidth(popup_t *popup, HDC *hdc)
{
  char *s;
  int a, b;
  int i, j;
  SIZE sz;

  a = 0;
  b = 0;

  //find body max pixel length
  SelectObject(*hdc, popup->body->font);

  s = popup->body->text;
  while (s && *s)
  {
    j = lstrlen(s);
    GetTextExtentExPoint(*hdc, s, j, POPUP_MAX_WIDTH, &i, NULL, &sz);
    if (i != j) //if it all wont fit...
    {
      j = i;
      while (i > 0) //...find the last space and add new line
      {
        if (s[i] == ' ')
        {
          s[i] = '\n';
          break;
        }
        i--;
      }
      if (!i) //didn't find a space, whack in a new line anyway
      {
        s[j] = '\n';
        i = j;
      }
      GetTextExtentPoint32(*hdc, s, i, &sz); //length...
      if (sz.cx > a) //...for the longest line we have
        a = sz.cx;
      s += i + 1;
    }
    else //it all fits on one line
    {
      if (!a || (sz.cx > a))
        a = sz.cx;
      break;
    }
  }

  //title
  SelectObject(*hdc, popup->title->font);

  s = popup->title->text;
  if (s)
  {
    GetTextExtentExPoint(*hdc, s, lstrlen(s), POPUP_MAX_WIDTH, &i, NULL, &sz);
    GetTextExtentPoint32(*hdc, s, i, &sz);
    b = sz.cx;
  }

#ifdef _VERBOSE
  yay("CalcTextWidth() Text Width: %d, Body Width: %d, MAX: %d: %s", a, b, POPUP_MAX_WIDTH, popup->name);
#endif

  if (b < a)
    return a;

  return b;
}

int CalcTextHeight(popup_t *popup, int width, int *titleheight, HDC *hdc)
{
  RECT rc;
  int height = 0;

  if (popup->title->text)
  {
    SelectObject(*hdc, popup->title->font);
    rc.top = 0;
    rc.bottom = 0;
    rc.left = 0;
    rc.right = width;
    height = DrawText(*hdc, popup->title->text, -1, &rc, DT_CALCRECT | DT_END_ELLIPSIS | DT_NOPREFIX);
  }
  *titleheight = height;

#ifdef _VERBOSE
  //yay("CalcTextHeight() title: %d, titleheight %d: %s", height, *titleheight, popup->name);
#endif

  if (popup->body->text)
  {
    SelectObject(*hdc, popup->body->font);
    rc.top = 0;
    rc.bottom = 0;
    rc.left = 0;
    rc.right = width;
    height += DrawText(*hdc, popup->body->text, -1, &rc, DT_CALCRECT | DT_END_ELLIPSIS | DT_NOPREFIX);
  }

  if (popup->title->text && popup->body->text)
  {
    height += POPUP_SEPERATOR;
  }

#ifdef _VERBOSE
  //yay("CalcTextHeight() all: %d, titleheight %d: %s", height, *titleheight, popup->name);
#endif

  return height;
}

void PrintText(popup_t *popup, int titleheight, RECT *di, HDC *hdc)
{
  RECT rc;

  //title
  if (popup->title->text)
  {
    SelectObject(*hdc, popup->title->font);
    SetTextColor(*hdc, popup->title->fontcolour);
    rectcpy(&rc, di);
    rc.left += POPUP_PADDING;
    rc.top += POPUP_PADDING;
    rc.right -= POPUP_PADDING;
    rc.bottom = rc.top + titleheight;

    //DrawText(*hdc, eek("%dx%d", rc.right - rc.left, rc.bottom - rc.top), -1, &rc, DT_END_ELLIPSIS | DT_NOPREFIX);
    //FillRect(*hdc, &rc, (HBRUSH) GetStockObject(WHITE_BRUSH));
    DrawText(*hdc, popup->title->text, -1, &rc, DT_END_ELLIPSIS | DT_NOPREFIX | DT_NOCLIP);
  }

  //text
  if (popup->body->text)
  {
    SelectObject(*hdc, popup->body->font);
    SetTextColor(*hdc, popup->body->fontcolour);
    rectcpy(&rc, di);
    if (titleheight)
    {
#ifdef _VERBOSE
      //yay("PrintText() Adding seperator: %s", popup->name);
#endif
      rc.top += titleheight + POPUP_SEPERATOR;
    }
    rc.left += POPUP_PADDING;
    rc.top += POPUP_PADDING;
    rc.right -= POPUP_PADDING;
    rc.bottom -= POPUP_PADDING;

    //DrawText(*hdc, eek("%dx%d", rc.right - rc.left, rc.bottom - rc.top), -1, &rc, DT_END_ELLIPSIS | DT_NOPREFIX);
    //FillRect(*hdc, &rc, (HBRUSH) GetStockObject(BLACK_BRUSH));
    DrawText(*hdc, popup->body->text, -1, &rc, DT_END_ELLIPSIS | DT_NOPREFIX | DT_NOCLIP);
  }

#ifdef _VERBOSE
  yay("PrintText() End: %s", popup->name); //v
#endif
}

int AddPopup(popup_t **popup)
{
  //doesn't matter if some fail here, everything gets free'd later on
  //ALLOC_MEM zero's memory
  *popup = (popup_t *)ALLOC_MEM(sizeof(popup_t));
  if (!*popup)
  {
#ifdef _VERBOSE
    yay("AddPopup() Memory error");
#endif
    return ARISE_OUT_OF_MEMORY;
  }

  (*popup)->title = (item_t *)ALLOC_MEM(sizeof(item_t));
  if (!(*popup)->title)
  {
#ifdef _VERBOSE
    yay("AddPopup() Memory error");
#endif
    return ARISE_OUT_OF_MEMORY;
  }

  (*popup)->body = (item_t *)ALLOC_MEM(sizeof(item_t));
  if (!(*popup)->body)
  {
#ifdef _VERBOSE
    yay("AddPopup() Memory error");
#endif
    return ARISE_OUT_OF_MEMORY;
  }

  EnterCriticalSection(cs);

  if (popups)
    popups->prev = *popup;
  (*popup)->next = popups;
  (*popup)->prev = NULL; //this is set next time AddPopup() is called
  popups = *popup;

  LeaveCriticalSection(cs);

#ifdef _VERBOSE
  yay("AddPopup() finished OK");
#endif

  return ARISE_NO_ERROR;
}

void FreePopup(popup_t *popup)
{
  if (!popup)
    return;

  if (popup->title)
  {
    if (popup->title->text)
      FREE_MEM(popup->title->text);
    if (popup->title->fontname)
      FREE_MEM(popup->title->fontname);
    if (popup->title->font)
      DeleteObject(popup->title->font);
    FREE_MEM(popup->title);
  }


  if (popup->body)
  {
    if (popup->body->text)
      FREE_MEM(popup->body->text);
    if (popup->body->fontname)
      FREE_MEM(popup->body->fontname);
    if (popup->body->font)
      DeleteObject(popup->body->font);
    FREE_MEM(popup->body);
  }

  if (popup->bgcolour)
    DeleteObject(popup->bgcolour);

  FREE_MEM(popup);
}

void RemovePopup(popup_t *popup)
{
#ifdef _VERBOSE
  yay("RemovePopup()");
#endif

  if (!popup)
    return;

  EnterCriticalSection(cs);

  if (popup->prev)
  {
#ifdef _VERBOSE
    yay("RemovePopup() Removing: %s, Prev Linking \"%s\" = \"%s\"", popup->name, (popup->prev) ? popup->prev->name : "(NULL)", popup->name);
#endif
    popup->prev->next = popup->next;
  }
  if (popup->next)
  {
#ifdef _VERBOSE
    yay("RemovePopup() Removing: %s, Next Linking \"%s\" = \"%s\"", popup->name, (popup->next) ? popup->next->name : "(NULL)", popup->name);
#endif
    popup->next->prev = popup->prev;
  }
  if (!popup->prev)
  {
#ifdef _VERBOSE
    yay("RemovePopup() Removing: %s, Last popup, popups = \"%s\"", popup->name, (popup->next) ? popup->next->name : "(NULL)");
#endif
    popups = popup->next;
  }

  FreePopup(popup);

  LeaveCriticalSection(cs);
}

void RemovePopups(void)
{
  popup_t *popup;

  while (popups)
  {
    popup = popups;

    if (popup->thread)
    {
#ifdef _VERBOSE
      yay("RemovePopups() killing thread: %s", popup->name);
#endif
      TerminateThread(popup->thread, 0);
      CloseHandle(popup->thread);
    }

    popups = popups->next;

    FreePopup(popup);
  }
}

#ifdef LEGACY_WIN9X
void UsrSetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
  if (pSetLayeredWindowAttributes)
  {
#ifdef _VERBOSE
    yay("pSetLayeredWindowAttributes(%d %d %d %d)", hwnd, crKey, bAlpha, dwFlags);
#endif
    pSetLayeredWindowAttributes(hwnd, crKey, bAlpha, dwFlags);
  }
}
#endif

#ifdef LEGACY_WIN9X
void UsrAnimateWindow(HWND hwnd, DWORD dwTime, DWORD dwFlags)
{
  if (pAnimateWindow)
    pAnimateWindow(hwnd, dwTime, dwFlags);
}
#endif

void Init(HINSTANCE inst)
{
  hinst = inst;
  cs = NULL;
  popups = NULL;
  wc = NULL;
#ifdef LEGACY_WIN9X
  usr = NULL;
  pSetLayeredWindowAttributes = NULL;
  pAnimateWindow = NULL;
#endif
}

int Begin(void)
{
  if (!wc)
  {
    if (!(wc = (WNDCLASS *) ALLOC_MEM(sizeof(WNDCLASS))))
      return ARISE_MASSIVE_FAILURE;

    wc->style = 0;
    wc->lpfnWndProc = &WndProc;
    wc->cbClsExtra = 0;
    wc->cbWndExtra = 0;
    wc->hInstance = hinst;
    wc->hIcon = NULL;
    wc->hCursor = LoadCursor(NULL, IDC_ARROW);
    wc->hbrBackground = NULL;
    wc->lpszMenuName = NULL;
    wc->lpszClassName = POPUP_CLASS;

    if (!RegisterClass(wc))
    {
#ifdef _VERBOSE
      yay("RegisterClass() failed");
#endif
      return ARISE_MASSIVE_FAILURE;
    }
  }

  if (!cs)
  {
    if (!(cs = (CRITICAL_SECTION *) ALLOC_MEM(sizeof(CRITICAL_SECTION))))
      return ARISE_MASSIVE_FAILURE;
    InitializeCriticalSection(cs);
  }

#ifdef LEGACY_WIN9X
  if (!usr)
    usr = LoadLibrary("user32");
  if (usr)
  {
    if (!pSetLayeredWindowAttributes)
      pSetLayeredWindowAttributes = (PSLWA)GetProcAddress(usr, "SetLayeredWindowAttributes");

    if (!pAnimateWindow)
      pAnimateWindow = (ANIWIN)GetProcAddress(usr, "AnimateWindow");
  }
#endif

  return ARISE_NO_ERROR;
}

void Finish(void)
{
  RemovePopups();

  if (cs)
  {
    DeleteCriticalSection(cs);
    FREE_MEM(cs);
  }

#ifdef LEGACY_WIN9X
  if (usr)
    FreeLibrary(usr);
#endif

  if (wc)
  {
    UnregisterClass(POPUP_CLASS, hinst);
    FREE_MEM(wc);
  }
}

#ifdef _VERBOSE
#define DEBUG_CLASS "Arise Debug Window"

LRESULT CALLBACK InfoWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_CREATE:
      return 1;
      break;
    case WM_SIZE:
      {
        RECT rcClient;

        GetClientRect(hwnd, &rcClient);
        SetWindowPos(hedit, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
      }
      break;
    case WM_CLOSE:
      DestroyWindow(hwnd);
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      break;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

void *InfoWindow(void)
{
  MSG msg;
  HDC hdc;
  HFONT font;
  WNDCLASS wc;
  long height;

  wc.style = 0;
  wc.lpfnWndProc = InfoWndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hinst;
  wc.hIcon = NULL;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = DEBUG_CLASS;

  if (!RegisterClass(&wc))
  {
#ifdef _VERBOSE
    MessageBox(NULL, "RegisterClass() failed", DEBUG_CLASS, MB_OK | MB_ICONEXCLAMATION);
#endif
    return 0;
  }

  hdebug = CreateWindowEx(/*WS_EX_TOPMOST*/0, DEBUG_CLASS, "Debug", WS_OVERLAPPEDWINDOW, 16, 76,
                        781, 446, NULL, NULL, hinst, NULL);
  if (!hdebug)
  {
    MessageBox(NULL, "CreateWindowEx() failed", DEBUG_CLASS, MB_OK | MB_ICONEXCLAMATION);
    return 0;
  }

  hedit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE |
                        ES_AUTOVSCROLL | ES_AUTOHSCROLL, 0, 0, 100, 100, hdebug, NULL, GetModuleHandle(NULL), NULL);
  if (!hedit)
  {
    MessageBox(NULL, "CreateWindowEx() failed", DEBUG_CLASS, MB_OK | MB_ICONEXCLAMATION);
    return 0;
  }

  hdc = GetDC(hdebug);
  height = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  ReleaseDC(hdebug, hdc);

  font = CreateFont(height, 0, 0, 0, 400, 0, 0, 0,
    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
  if (!font)
  {
    MessageBox(NULL, "CreateFont() failed", DEBUG_CLASS, MB_OK | MB_ICONEXCLAMATION);
    font = GetStockObject(SYSTEM_FONT);
  }
  SendMessage(hedit, WM_SETFONT, (WPARAM)font, (LPARAM)0);

  ShowWindow(hdebug, SW_SHOW);
  UpdateWindow(hdebug);

  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  UnregisterClass(DEBUG_CLASS, hinst);
  DeleteObject(font);
  CloseHandle(thdebug);

  return 0;
}

void yay(char *fmt, ...)
{
  va_list argptr;
  char msg[4096];
  int len;

  va_start(argptr, fmt);
  _vsnprintf(msg, sizeof(msg)-4, fmt, argptr);
  va_end(argptr);
  lstrcat(msg, "\x0D\x0A");

  if (!hdebug || !hedit)
  {
    //MessageBox(NULL, msg, "Debug", MB_OK);
    return;
  }

  len = GetWindowTextLength(hedit);
  SendMessage(hedit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
  SendMessage(hedit, EM_REPLACESEL, 0, (LPARAM)msg);
}

char *eek(char *fmt, ...)
{
  static char msg[4096];
  va_list argptr;

  va_start(argptr, fmt);
  _vsnprintf(msg, sizeof(msg), fmt, argptr);
  va_end(argptr);
  msg[sizeof(msg)-1] = '\0';

  return msg;
}

/*
void dump(popup_t *popup)
{
  yay("popup->bfont %d", popup->bfont);
  yay("popup->tfont %d", popup->tfont);
  yay("popup->bgcolour %d", popup->bgcolour);
  yay("popup->hwnd %d", popup->hwnd);
  yay("popup->id %d", popup->id);
  yay("popup->name %d", popup->name);
  yay("popup->repaint %d", popup->repaint);
  yay("popup->pos %d", popup->pos);
  yay("popup->next %d", popup->next);
  yay("popup->prev %d", popup->prev);

  yay("popup->time %d", popup->time);
  yay("popup->winop %d", popup->winop);
  yay("popup->alpha %d", popup->alpha);
  yay("popup->showani %d", popup->showani);
  yay("popup->showtime %d", popup->showtime);
  yay("popup->hideani %d", popup->hideani);
  yay("popup->hidetime %d", popup->hidetime);
  yay("popup->bgcolour %d", popup->bgcolour);

  yay("popup->title->text %s", popup->title->text);
  yay("popup->title->fontname %s", popup->title->fontname);
  yay("popup->title->fontheight %d", popup->title->fontheight);
  yay("popup->title->fontstyle %d", popup->title->fontstyle);
  yay("popup->title->fontcolour %d", popup->title->fontcolour);

  yay("popup->body->text %s", popup->body->text);
  yay("popup->body->fontname %s", popup->body->fontname);
  yay("popup->body->fontheight %d", popup->body->fontheight);
  yay("popup->body->fontstyle %d", popup->body->fontstyle);
  yay("popup->body->fontcolour %d", popup->body->fontcolour);
}
*/
#endif

int aniflag(int flag)
{
  int i = 0;

  if (flag & EFFECT_ANIMATE_FADE)
    i |= AW_BLEND;
  if (flag & EFFECT_ANIMATE_CENTER)
    i |= AW_CENTER;
  //if (flag & EFFECT_ANIMATE_ROLL) //default
  //  i |= AW_ROLL;
  if (flag & EFFECT_ANIMATE_SLIDE)
    i |= AW_SLIDE;
  if (flag & EFFECT_ANIMATE_LEFT_TO_RIGHT)
    i |= AW_HOR_POSITIVE;
  if (flag & EFFECT_ANIMATE_RIGHT_TO_LEFT)
    i |= AW_HOR_NEGATIVE;
  if (flag & EFFECT_ANIMATE_TOP_TO_BOTTOM)
    i |= AW_VER_POSITIVE;
  if (flag & EFFECT_ANIMATE_BOTTOM_TO_TOP)
    i |= AW_VER_NEGATIVE;

  return i;
}

int _atoi(char *str)
{
  int val;
  int sign;
  int c;

  if (*str != '-')
    sign = 1;
  else
  {
    sign = -1;
    str++;
  }

  val = 0;

  while (1)
  {
    c = *str++;
    if (c < '0' || c > '9')
      return val * sign;
    val = val * 10 + c - '0';
  }

  return 0;
}

COLORREF readrgb(const char *str)
{
  char *buf, *s, *p;
  int rgb[3];
  int len, i;

  rgb[0] = 0;
  rgb[1] = 0;
  rgb[2] = 0;

  len = lstrlen(str);
  buf = (char *)ALLOC_MEM(len + 1);
  if (buf)
  {
    lstrcpy(buf, str);
    s = buf;

    for (i = 0, p = s; (i < 3) && *s; i++)
    {
      while (*s && *s != ',')
        s++;

      *s++ = '\0';
      rgb[i] =  _atoi(p);
      p = s;
    }

    FREE_MEM(buf);
  }

  return (RGB(rgb[0], rgb[1], rgb[2]));
}

int clamp(int min, int max, int n)
{
  if (n > max)
    return max;
  if (n < min)
    return min;
  return n;
}

void rectcpy(RECT *dst, RECT *src)
{
  dst->top = src->top;
  dst->bottom = src->bottom;
  dst->left = src->left;
  dst->right = src->right;
}

#ifdef _VERBOSE
/*  $OpenBSD: strlcat.c and strlcpy.c,v 1.11 2003/06/17 21:56:24 millert Exp $
 *
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
  register char *d = dst;
  register const char *s = src;
  register size_t n = siz;

  /* Copy as many bytes as will fit */
  if (n != 0 && --n != 0) {
    do {
      if ((*d++ = *s++) == 0)
        break;
    } while (--n != 0);
  }

  /* Not enough room in dst, add NUL and traverse rest of src */
  if (n == 0) {
    if (siz != 0)
      *d = '\0';    /* NUL-terminate dst */
    while (*s++)
      ;
  }

  return(s - src - 1);  /* count does not include NUL */
}
#endif

#ifdef _DEBUG
void __declspec (naked) __cdecl _chkesp (void)
{
  _asm { jz exit_chkesp };
  _asm { int 3          };
  exit_chkesp:
  _asm { ret            };
}
#endif
