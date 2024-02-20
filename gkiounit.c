/*
  Graphics and Keyboard Input/Output Unit
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __MSDOS__
#include <alloc.h>
#include <dos.h>
#endif
#ifdef __WIN32__
#include <windows.h>
#include "win32api.h"
#endif

#include "gkiounit.h"

/* bitmap header: BITMAPFILEHEADER + BITMAPINFOHEADER + PALETTE[2] */
#define BMP_HDR_SIZE 62
/* bitmap data ((640/8)*480) */
#define BMP_BUF_SIZE 38400U

#ifdef __MSDOS__
static unsigned char far *scrmem = MK_FP(0xA000, 0x0000);
static unsigned char PrevVidMode = 0;
#endif

#ifdef __WIN32__
/*
  note that the biHeight is negative, so the bitmap is a top-down DIB and its origin is the upper left corner
*/
static unsigned char bmphdr[BMP_HDR_SIZE] = {
  0x42, 0x4D, 0x3E, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x28, 0x00,
  0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0x20, 0xFE, 0xFF, 0xFF, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
};
/* offscreen buffer - BMP file size */
static unsigned char scrbuf[BMP_BUF_SIZE + BMP_HDR_SIZE];
static unsigned char *scrmem = NULL;
static unsigned int lastchar = 0;
HWND wnd = 0;
HANDLE th;
HANDLE eh;
BOOL IsWndMode = FALSE;
#endif

static unsigned char *GraphFont = NULL;
static unsigned char GraphColor = 0;

#ifdef __MSDOS__
unsigned int x_kbhit(void) {
union REGS regs;
  regs.h.ah = 0x01;
  int86(0x16, &regs, &regs);
  /* 6 - zero flag is set when buffer empty */
  return((regs.x.flags & 0x40) ? 0 : 1);
}

unsigned int x_getch(void) {
union REGS regs;
  regs.h.ah = 0x00;
  int86(0x16, &regs, &regs);
  return(regs.x.ax);
}
#endif

#ifdef __WIN32__
unsigned int x_kbhit(void) {
  /*
    this was allow to go through all possible loops and checks
    so the KB_ESC will be readed by the calling of x_getch()
  */
  if (lastchar & 2) {
    lastchar ^= 4;
  }
  if (lastchar & 4) {
    return(0);
  }
  return(lastchar ? 1 : 0);
}

unsigned int x_getch(void) {
unsigned int x;
  if (eh && (!lastchar)) {
    WaitForSingleObject(eh, INFINITE);
  }
  x = lastchar;
  if (LOBYTE(lastchar) == 1) {
    lastchar = 0;
  }
  return(x);
}
#endif

/*
  http://atrevida.comprenica.com/atrtut19.html 
  http://alexfru.narod.ru/emiscdocs.html
  http://alexfru.chat.ru/
*/

/*
  all this needed to drop graphics files
  EGAVGA.BGI and GRAPHICS.H and reduce
  final program size since there are no
  need in color graphics anyway
*/

#ifdef __MSDOS__
void SetVideoMode(unsigned char mode) {
union REGS regs;
  regs.h.ah = 0x00;
  regs.h.al = mode;
  int86(0x10, &regs, &regs);
}

int IsVGADisplay(void) {
union REGS regs;
  regs.h.ah = 0x1A;
  regs.h.al = 0x00;
  int86(0x10, &regs, &regs);
  /* test VGA support
     AL = 0x1A - function supported
     BL - active display, BH - inactive display
     values:
       7 - VGA mono
       8 - VGA color
       more - MCGA
       0xFF - unsupported
  */
  return(((regs.h.al == 0x1A) && (regs.h.bl >= 0x07) && (regs.h.bl != 0xFF)) ? 1 : 0);
}

unsigned char GetVideoMode(void) {
union REGS regs;
  regs.h.ah = 0x0F;
  int86(0x10, &regs, &regs);
  return(regs.h.al);
}

void ClearScreen(void) {
unsigned int i;
  /* Turbo C 2.01 doesn't have
   _fmemset() for far pointers */
  for (i = 0; i < BMP_BUF_SIZE; i++) {
    scrmem[i] = GraphColor;
  }
}

int InitGraphics(void) {
int result;
  result = 2;
  /* not in graphic mode */
  if (!PrevVidMode) {
    result = 0;
    /* test for VGA support */
    if (IsVGADisplay()) {
      /* save current vid mode
         add 1 for zero mode */
      PrevVidMode = GetVideoMode() + 1;
      /* set mode 640x480x2 (B/W) */
      SetVideoMode(0x11);
      ClearScreen();
      result = 1;
    }
  }
  return(result);
}

void FreeGraphics(void) {
  /* in graphic mode */
  if (PrevVidMode) {
    /* restore vid mode */
    SetVideoMode(PrevVidMode - 1);
    GraphColor = 0;
    PrevVidMode = 0;
    GraphFont = NULL;
  }
}
#endif

#ifdef __WIN32__
/* draw screen buffer to window frame */
void BufferToScreen(HWND wnd) {
HBITMAP hOld, hBack;
PAINTSTRUCT ps;
HDC hc, hMem;
RECT rc;
  hc = BeginPaint(wnd, &ps);
  hMem = CreateCompatibleDC(hc);
  hBack = CreateDIBitmap(
    hc, (BITMAPINFOHEADER *)(&scrbuf[sizeof(BITMAPFILEHEADER)]),
    CBM_INIT, (void *)(&scrbuf[BMP_HDR_SIZE]),
    (BITMAPINFO *)(&scrbuf[sizeof(BITMAPFILEHEADER)]), DIB_RGB_COLORS
  );
  if (hBack) {
    hOld = SelectObject(hMem, hBack);
    GetClientRect(wnd, &rc);
    BitBlt(hc, rc.left, rc.top, rc.right, rc.bottom, hMem, 0, 0, SRCCOPY);
    SelectObject(hMem, hOld);
    DeleteObject(hBack);
  }
  DeleteDC(hMem);
  EndPaint(wnd, &ps);
}

void DumpScreen(void) {
TCHAR s[16];
HANDLE fl;
DWORD i;
  /* find non-existent file */
  for (i = 1; i; i++) {
    wsprintf(s, "%08u.bmp", i);
    /* file not exists */
    if (GetFileAttributes(s) == INVALID_FILE_ATTRIBUTES) {
      fl = CreateFile(s, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
      if (fl != INVALID_HANDLE_VALUE) {
        WriteFile(fl, scrbuf, sizeof(scrbuf), &i, NULL);
        CloseHandle(fl);
      }
      break;
    }
  }
}

LRESULT CALLBACK WndPrc(HWND wnd, UINT umsg, WPARAM wparm, LPARAM lparm) {
DEVMODE dm;
RECT rc;
  switch (umsg) {
    case WM_CREATE:
      /* if Shift pressed - run in windowed mode */
      if (!IsWndMode) {
        /* go to the fullscreen */
        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmPelsWidth  = VID_MAX_X + 1;
        dm.dmPelsHeight = VID_MAX_Y + 1;
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
      } else {
        rc.top = 0;
        rc.left = 0;
        rc.right = VID_MAX_X + 1;
        rc.bottom = VID_MAX_Y + 1;
        AdjustWindowRect(&rc, GetWindowLong(wnd, GWL_STYLE), FALSE);
        rc.bottom -= rc.top;
        rc.right -= rc.left;
        MoveWindow(wnd,
          (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
          (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
          rc.right,
          rc.bottom,
          TRUE
        );
      }
      return(0);
      break;
    case WM_SETCURSOR:
      /* hide cursor for client area in fullscreen mode */
      if (!IsWndMode) {
        SetCursor((LOWORD(lparm) == HTCLIENT) ? NULL : LoadCursor(NULL, IDC_ARROW));
        return(TRUE);
      }
      break;
    case WM_KEYDOWN:
      if (wparm == VK_F12) { DumpScreen(); }
      if (!lastchar) {
        lastchar = MAKEWORD(1, LOBYTE(HIWORD(lparm)));
        SetEvent(eh);
      }
      break;
    case WM_PAINT:
      BufferToScreen(wnd);
      return(0);
      break;
    case WM_DESTROY:
      if (!IsWndMode) {
        /* restore vid mode */
        ChangeDisplaySettings(NULL, 0);
      }
      /* drop keyboard event */
      lastchar = MAKEWORD(2, KB_ESC);
      SetEvent(eh);
      PostQuitMessage(0);
      return(0);
    break;
  }
  return(DefWindowProc(wnd, umsg, wparm, lparm));
}

DWORD WINAPI WndLoopThreadFunc(LPVOID parm) {
WNDCLASSEX wcex;
MSG wmsg;
  IsWndMode = GetAsyncKeyState(VK_SHIFT) ? TRUE : FALSE;
  if (!IsWndMode) {
    IsWndMode = MessageBox(0, "Run in fullscreen mode?", "Confirm", MB_YESNO | MB_ICONQUESTION) == IDNO;
  }
  /* create window */
  ZeroMemory(&wcex, sizeof(wcex));
  wcex.cbSize = sizeof(wcex);
  wcex.lpszClassName = "{6B8C783A-9676-47D5-B91A-DA0ABFFD6C3B}";
  wcex.hInstance     = GetModuleHandle(NULL);
  wcex.lpfnWndProc   = WndPrc;
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
  if (!RegisterClassEx(&wcex)) {
    SetEvent(eh);
    return(1);
  }
  wnd = CreateWindowEx(
    0, wcex.lpszClassName, "TPROGRAM",
    /* if Shift pressed - run in windowed mode */
    (!IsWndMode) ? (WS_VISIBLE | WS_SYSMENU | WS_POPUP) :
      (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE),
    0,
    0,
    VID_MAX_X + 1, VID_MAX_Y + 1,
    0, 0,
    wcex.hInstance, NULL
  );
  if (!wnd) {
    SetEvent(eh);
    return(2);
  }
  /* signaling that everything alright */
  SetEvent(eh);
  while (GetMessage(&wmsg, 0, 0, 0)) {
    TranslateMessage(&wmsg);
    DispatchMessage(&wmsg);
  }
  UnregisterClass(wcex.lpszClassName, wcex.hInstance);
  wnd = 0;
  return(0);
}

void ClearScreen(void) {
  if (scrmem) {
    FillMemory(scrmem, BMP_BUF_SIZE, GraphColor);
    InvalidateRect(wnd, NULL, TRUE);
  }
}

int InitGraphics(void) {
int result;
DWORD tid;
  result = 2;
  /* not in graphic mode */
  if (!scrmem) {
    result = 0;
    eh = CreateEvent(NULL, FALSE, FALSE, NULL);
    /* create windows loop thread */
    th = CreateThread(NULL, 0, &WndLoopThreadFunc, NULL, 0, &tid);
    if (th) {
      WaitForSingleObject(eh, INFINITE);
      if (wnd) {
        /* init bitmap data */
        memmove(scrbuf, bmphdr, BMP_HDR_SIZE);
        /* offset to the actual bitmap data */
        scrmem = &scrbuf[BMP_HDR_SIZE];
        result = 1;
        ClearScreen();
      }
    }
    if (!wnd) {
      CloseHandle(eh);
      eh = NULL;
    }
  }
  return(result);
}

void FreeGraphics(void) {
DWORD ts;
  /* in graphic mode */
  if (scrmem) {
    /* close and destroy window */
    if (wnd) {
      SendMessage(wnd, WM_CLOSE, 0, 0);
    }
    if (GetExitCodeThread(th, &ts) && (ts == STILL_ACTIVE)) {
      /* wait until everything cleaned up */
      WaitForSingleObject(th, INFINITE);
    }
    CloseHandle(th);
    /* keyboard event */
    CloseHandle(eh);
    eh = 0;
    /* cleanup */
    scrmem = NULL;
  }
}
#endif

void DrawPixel(int x, int y) {
  if ((x >= 0) && (y >= 0) && (x <= VID_MAX_X) && (y <= VID_MAX_Y)) {
    if (GraphColor) {
      scrmem[(y * 80) + (x >> 3)] |= (128 >> (x & 7));
    } else {
      scrmem[(y * 80) + (x >> 3)] &= ~(128 >> (x & 7));
    }
  }
}

/* Bresenhem algo */
void DrawLineX(int x1, int y1, int x2, int y2) {
int i, dx, dy, sx, sy, d1, d2, dd;
  /* flip line */
  if (y1 > y2) {
    y1 ^= y2;
    y2 ^= y1;
    y1 ^= y2;
  }
  /* initialize */
  dx = x2 - x1;
  dx = (dx < 0) ? (-dx) : (dx);
  dy = y2 - y1;
  sx = 1;
  if (x1 > x2) { sx = -1; }
  if (x1 == x2) { sx = 0; }
  sy = 1;
  if (dx >= dy) {
    /* generate dx >= dy */
    d1 = dy + dy;
    dd = d1 - dx;
    d2 = dd - dx;
    for (i = 0; i <= dx; i++) {
      DrawPixel(x1, y1);
      x1 += sx;
      if (dd < 0) {
        dd += d1;
      } else {
        dd += d2;
        y1 += sy;
      }
    }
  } else {
    /* generate dx < dy */
    d1 = dx + dx;
    dd = d1 - dy;
    d2 = dd - dy;
    for (i = 0; i <= dy; i++) {
      DrawPixel(x1, y1);
      y1 += sy;
      if (dd < 0) {
        dd += d1;
      } else {
        dd += d2;
        x1 += sx;
      }
    }
  }
}

void DrawTextX(int x, int y, char *s, int len) {
unsigned char *h, b;
int i, j, k;
  if (GraphFont) {
    for (k = 0; k < len; k++) {
      h = GraphFont;
      h += ((unsigned char) s[k]) << 3; /* *8 */
      for (j = 0; j < 8; j++) {
        b = 128;
        for (i = 0; i < 8; i++) {
          if (*h & b) {
            DrawPixel(x + i, y);
          }
          b >>= 1;
        }
        y++;
        h++;
      }
      y -= 8;
      x += 8;
    }
  }
#ifdef __WIN32__
  InvalidateRect(wnd, NULL, TRUE);
#endif
}

void DrawRectX(int x1, int y1, int x2, int y2) {
  DrawLineX(x1, y1, x2, y1);
  DrawLineX(x2, y1, x2, y2);
  DrawLineX(x2, y2, x1, y2);
  DrawLineX(x1, y2, x1, y1);
#ifdef __WIN32__
  InvalidateRect(wnd, NULL, TRUE);
#endif
}

/* very slow, but just fine for small areas */
void FillRectX(int x1, int y1, int x2, int y2) {
int i;
  /* flip */
  if (y1 > y2) {
    y1 ^= y2;
    y2 ^= y1;
    y1 ^= y2;
  }
  for (i = y1; i <= y2; i++) {
    DrawLineX(x1, i, x2, i);
  }
#ifdef __WIN32__
  InvalidateRect(wnd, NULL, TRUE);
#endif
}

void SetGraphColor(unsigned char color) {
  GraphColor = color ? 0xFF : 0;
}

void SetActiveFont(unsigned char *font) {
  GraphFont = font;
}

/* image routines */

void PutMonoImage(unsigned int x, unsigned int y, unsigned char *p) {
unsigned int i, sw, iw, ih, lw;
  if (p) {
    iw = (p[19] << 8) | p[18];
    iw = (iw / 8) + ((iw % 8) ? 1 : 0);
    lw = iw + ((4 - (iw & 3)) & 3);
    ih = (p[23] << 8) | p[22];
    sw = (VID_MAX_X + 1) / 8;
    y += ih - 1;
    y *= sw;
    y += (x / 8);
    p += BMP_HDR_SIZE;
    for (i = 0; i < ih; i++) {
#ifdef __MSDOS__
      movedata(_DS, (unsigned) &p[i * lw], 0xA000, y - (sw * i), iw);
#endif
#ifdef __WIN32__
      memmove(&scrmem[y - (sw * i)], &p[i * lw], iw);
#endif
    }
  }
#ifdef __WIN32__
  InvalidateRect(wnd, NULL, TRUE);
#endif
}

unsigned char *GetMonoImage(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
unsigned char *p;
unsigned int i, sw, iw, lw, sz;
  p = NULL;
  w &= 0xFFFF;
  h &= 0xFFFF;
  if (w && h) {
    iw = (w / 8) + ((w % 8) ? 1 : 0);
    lw = iw + ((4 - (iw & 3)) & 3);
    sz = (lw * h) + BMP_HDR_SIZE;
    p = (unsigned char *) malloc(sz);
    if (p) {
      memset(p, 0, sz);
      sw = (VID_MAX_X + 1) / 8;
      y += h - 1;
      y *= sw;
      y += (x / 8);
      p += BMP_HDR_SIZE;
      for (i = 0; i < h; i++) {
#ifdef __MSDOS__
        movedata(0xA000, y - (sw * i), _DS, (unsigned) &p[i * lw], iw);
#endif
#ifdef __WIN32__
      memmove(&p[i * lw], &scrmem[y - (sw * i)], iw);
#endif
      }
      p -= BMP_HDR_SIZE;
      p[18] = w & 0xFF;
      p[19] = w >> 8;
      p[22] = h & 0xFF;
      p[23] = h >> 8;
    }
  }
  return(p);
}
