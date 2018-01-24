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

//#define LEGACY_SUPPORT
//#define LEGACY_VERSION_1
//#define LEGACY_WIN9X

#ifndef LEGACY_WIN9X
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include "AggressiveOptimize.h"
#ifdef _VERBOSE
#include <stdarg.h>
#include <stdio.h>
#endif

#define HEADER_NAME "Arise"
#define HEADER_VER "v2.0"
#define HEADER_AUTHOR "bliP"
#define HEADER_EMAIL "spawn[at]nisda.net"
#define HEADER_COPYRIGHT "(C) 2004/2006/2007"
#define HEADER_WEBSITE "http://nisda.net"

#ifdef _VERBOSE
#define HEADER_VEREX "D"
#else
#ifdef LEGACY_WIN9X
#define HEADER_VEREX "L"
#else
#define HEADER_VEREX ""
#endif
#endif

#define POPUP_MAX_WIDTH 450
#define POPUP_PADDING 5
#define POPUP_BORDER 1
#define POPUP_SEPERATOR 3
#define POPUP_SPACE 2
#define POPUP_CLASS "Drunken Brawler"

#define POPUP_V1 1
#define POPUP_V1_NUM_DELIMITERS 12
#define POPUP_V1_DELIMITER -73 //250 (0xFA)

#define PARSE_V1_TITLE_FONTNAME 0
#define PARSE_V1_TITLE_FONTHEIGHT 1
#define PARSE_V1_TITLE_FONTSTYLE 2
#define PARSE_V1_TITLE_FONTCOLOUR 3
#define PARSE_V1_BODY_FONTNAME 4
#define PARSE_V1_BODY_FONTHEIGHT 5
#define PARSE_V1_BODY_FONTSTYLE 6
#define PARSE_V1_BODY_FONTCOLOUR 7
#define PARSE_V1_BACKGROUND_COLOUR 8
#define PARSE_V1_SHOW_TIME 9
#define PARSE_V1_TITLE_TEXT 10
#define PARSE_V1_BODY_TEXT 11

#define POPUP_V2 2
#define POPUP_V2_NUM_DELIMITERS 19
#define POPUP_V2_DELIMITER '_'
#define POPUP_V2_ESCAPE '\\'

#define PARSE_V2_TITLE_TEXT 0
#define PARSE_V2_BODY_TEXT 1
#define PARSE_V2_TITLE_FONTNAME 2
#define PARSE_V2_TITLE_FONTHEIGHT 3
#define PARSE_V2_TITLE_FONTSTYLE 4
#define PARSE_V2_TITLE_FONTCOLOUR 5
#define PARSE_V2_BODY_FONTNAME 6
#define PARSE_V2_BODY_FONTHEIGHT 7
#define PARSE_V2_BODY_FONTSTYLE 8
#define PARSE_V2_BODY_FONTCOLOUR 9
#define PARSE_V2_BACKGROUND_COLOUR 10
#define PARSE_V2_SHOW_TIME 11
#define PARSE_V2_WINDOW_OPTION 12
#define PARSE_V2_WINDOW_POSITION 13
#define PARSE_V2_TRANSPARENCY 14
#define PARSE_V2_ANIMATION_SHOW_EFFECTS 15
#define PARSE_V2_ANIMATION_SHOW_TIME 16
#define PARSE_V2_ANIMATION_HIDE_EFFECTS 17
#define PARSE_V2_ANIMATION_HIDE_TIME 18

#define TIMER_ID 50
#define TIMER_MAX (30 * 1000)

#define FONT_STYLE_BOLD 2
#define FONT_STYLE_ITALIC 4
#define FONT_STYLE_UNDERLINE 8

#define MIRC_HALT 0
#define MIRC_CONTINUE 1
#define MIRC_COMMAND 2
#define MIRC_IDENTIFIER 3

#define ALLOC_MEM(i) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, i)
#define FREE_MEM(p) HeapFree(GetProcessHeap(), 0, p)

#define ARISE_NO_ERROR 0
#define ARISE_OUT_OF_MEMORY 1
#define ARISE_BAD_DELIMITERS 2
#define ARISE_MISSING_TEXT 3
#define ARISE_MASSIVE_FAILURE 4

#define EFFECT_BORDER 1
#define EFFECT_POPUPLEFT 2
#define EFFECT_POPUPRIGHT 4
#define EFFECT_POPUPTOP 8
#define EFFECT_POPUPBOTTOM 16

#define EFFECT_ANIMATE_FADE 1
#define EFFECT_ANIMATE_CENTER 2
#define EFFECT_ANIMATE_ROLL 4
#define EFFECT_ANIMATE_SLIDE 8

#define EFFECT_ANIMATE_LEFT_TO_RIGHT 16
#define EFFECT_ANIMATE_RIGHT_TO_LEFT 32
#define EFFECT_ANIMATE_TOP_TO_BOTTOM 64
#define EFFECT_ANIMATE_BOTTOM_TO_TOP 128

#ifdef LEGACY_WIN9X
typedef DWORD (WINAPI *PSLWA)(HWND, DWORD, BYTE, DWORD);
typedef BOOL (WINAPI *ANIWIN)(HWND, DWORD, DWORD);

#define WS_EX_LAYERED 0x80000
#define LWA_COLORKEY 1
#define LWA_ALPHA 2

#define AW_HOR_POSITIVE 0x00000001
#define AW_HOR_NEGATIVE 0x00000002
#define AW_VER_POSITIVE 0x00000004
#define AW_VER_NEGATIVE 0x00000008
#define AW_CENTER 0x00000010
#define AW_HIDE 0x00010000
#define AW_ACTIVATE 0x00020000
#define AW_SLIDE 0x00040000
#define AW_BLEND 0x00080000
#endif

typedef struct
{
  DWORD mVersion;
  HWND mHwnd;
  BOOL mKeep;
} LOADINFO;

typedef struct item_s
{
  char *text;
  char *fontname;
  int fontstyle;
  long fontheight;
  COLORREF fontcolour;
  HFONT font;
} item_t;

typedef struct popup_s
{
  HWND hwnd;
  HANDLE thread;
  HBRUSH bgcolour;
  RECT pos;
  struct item_s *title;
  struct item_s *body;
  int time;
  int repaint;
  int winop;
  int point;
  int alpha;
  int showani;
  int showtime;
  int hideani;
  int hidetime;
  struct popup_s *next;
  struct popup_s *prev;
#ifdef _VERBOSE
  int id;
  char name[32];
#endif
} popup_t;

void Init(HINSTANCE inst);
int Begin(void);
void Finish(void);

int AddPopup(popup_t **popup);

int ParseDetect(char *data);
int ParsePopupV1(char *data);
int ParsePopupV2(char *data);

void RemovePopup(popup_t *popup);
void RemovePopups(void);
void FreePopup(popup_t *popup);

void *CreatePopup(LPVOID *data);
void DestroyPopup(HWND hwnd);

int CalcTextWidth(popup_t *popup, HDC *hdc);
int CalcTextHeight(popup_t *popup, int width, int *titleheight, HDC *hdc);
void PrintText(popup_t *popup, int titleheight, RECT *di, HDC *hdc);
void PaintPopup(HWND hwnd, PAINTSTRUCT *pps);

void PrintClient(HWND hwnd, HDC hdc);
void Paint(HWND hwnd);

#ifdef LEGACY_WIN9X
void UsrSetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
void UsrAnimateWindow(HWND hwnd, DWORD dwTime, DWORD dwFlags);
#else
#define UsrSetLayeredWindowAttributes SetLayeredWindowAttributes
#define UsrAnimateWindow AnimateWindow
#endif

LRESULT CALLBACK WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
int WINAPI _DllMainCRTStartup(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpvReserved);

COLORREF readrgb(const char *s);
int _atoi(char *str);
void hstrrep(char **str);
int clamp(int min, int max, int n);
void rectcpy(RECT *dst, RECT *src);
int aniflag(int flag);

#ifdef _VERBOSE
size_t strlcpy(char *dst, const char *src, size_t siz);
void yay(char *fmt, ...);
char *eek(char *fmt, ...);
//void dump(popup_t *popup);
void *InfoWindow(void);
LRESULT CALLBACK InfoWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
