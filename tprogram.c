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

#include "testunit.h"
#include "gkiounit.h"

#define FIRST_TESTS_HINT 3

static testdata *list;
static unsigned char *msgs;
static unsigned char *font;
static char *answ;

unsigned char GetKeyCode(void) {
unsigned int key;
  while (x_kbhit()) { x_getch(); }
  key = x_getch();
  return((key & 0xFF) ? (key >> 8) : 0);
}

void DrawTextInfo(int x, int y, int w, char *s) {
size_t i;
int j, k;
  if (s) {
    k = 0;
    j = 0;
    for (i = 0; i <= strlen(s); i++) {
      if ((s[i] == '\n') || (s[i] == '\0')) {
        if (k) {
          if (w >= 0)  {
            DrawTextX(x + ((w - (k * 8)) / 2) * (w ? 1 : 0), y + j, &s[i - k], k);
          } else {
            DrawTextX(x, y + j, &s[i - k], k);
            j++;
          }
        }
        j += 8;
        k = 0;
      } else {
        k++;
      }
    }
  }
}

void InitScreen(void) {
char *s;
  SetGraphColor(1);
  ClearScreen();
  SetGraphColor(0);
  /* show program info */
  s = (char *) GetFileFromArchive("TESTTEXT");
  if (s) {
    DrawTextInfo(16, 16, -1, s);
    free(s);
    DrawTextInfo(0, VID_MAX_Y - 24, VID_MAX_X, GetStrText(msgs, 5));
    GetKeyCode();
  }
  SetGraphColor(1);
  ClearScreen();
  SetGraphColor(0);
  DrawLineX(360, 0, 360, 430);
  DrawLineX(0, 209, 360, 209);
  DrawLineX(0, 430, 360, 430);
  DrawTextInfo(360, 10, 280, GetStrText(msgs, 0));
  DrawTextInfo(10, 435,   0, GetStrText(msgs, 1));
}

void DrawCheckMsg(int state) {
int i, x;
char *c;
  c = GetStrText(msgs, 2);
  x = (strlen(c) + 1) * 8;
  i = (280 - x) / 2;
  SetGraphColor(1);
  FillRectX(360 + i - 4, 206 - 4 - 1, 360 + i + x + 4, 206 + 8 + 4);
  if (state) {
    SetGraphColor(0);
    DrawRectX(360 + i - 4, 206 - 4 - 1, 360 + i + x + 4, 206 + 8 + 4);
    DrawTextInfo(360 + i + 4 + 1, 206, 0, c);
  }
}

void DrawScreen(int idx) {
unsigned char *p;
char name[8];
int i, x, k;
  /* load images */
  memset(name, 0, 8);
  strncpy(name, GET_TEST_NAME_FILE(list, idx), 8);
  x = 0;
  for (i = 0; i < 8; i++) {
    if (!name[i]) {
      x = i;
      break;
    }
  }
  for (i = 0; i < 2; i++) {
    name[x] = '0' + i;
    p = GetFileFromArchive(name);
    if (p) {
      PutMonoImage(8, 10 + (210*i), p);
      free(p);
    }
  }
  /* draw digits */
  x = 0;
  k = GET_TEST_IMG_ORDER(list, idx);
  for (i = 0; i < 9; i++) {
    *name = (char) 0xDB;
    SetGraphColor(1);
    DrawTextX(60 + ((i%3)*122), 274 + ((i/3)*70), name, 1);
    if (k&1) {
      x++;
      *name = '0' + x;
      SetGraphColor(0);
      DrawTextX(60 + ((i%3)*122), 274 + ((i/3)*70), name, 1);
    }
    k >>= 1;
  }
  /* clear prev status */
  SetGraphColor(1);
  FillRectX(360 + 1, 10 + 32, VID_MAX_X - 1, 10 + 56);
  SetGraphColor(0);
  /* draw status */
  sprintf(name, "%02d/%02d", (idx + 1) % 1000, list->count % 1000);
  DrawTextInfo(360, 10 + 32, 280, name);
  /* draw test name */
  DrawTextInfo(360, 10 + 48, 280, GET_TEST_NAME_TEXT(list, idx));
}

int PutTestImage(int idx, int num) {
unsigned char *p, *m;
int res, i, k, b, x;
  res = 0;
  k = GET_TEST_IMG_ORDER(list, idx);
  x = GET_TEST_BAR_PLACE(list, idx) - 1;
  b = 0;
  for (i = 0; i < 9; i++) {
    b += k&1;
    if (b == num) {
      p = GetMonoImage(8 + ((x%3)*120), 10 + ((x/3)*70), 112, 50);
      if (p) {
        m = GetMonoImage(8 + ((i%3)*120), 10 + ((i/3)*70) + 210, 112, 50);
        if (m) {
          PutMonoImage(8 + ((x%3)*120), 10 + ((x/3)*70), m);
          free(m);
        }
        if (GetKeyCode() == KB_ENTER) {
          res |= (GET_TEST_RIGHT_NUM(list, idx) == num) ? 1 : 0;
          res |= (idx < FIRST_TESTS_HINT) ? 0 : 2;
          answ[idx] = res&1;
          if (!res) {
            DrawCheckMsg(1);
            GetKeyCode();
            DrawCheckMsg(0);
          }
        }
        PutMonoImage(8 + ((x%3)*120), 10 + ((x/3)*70), p);
        free(p);
      }
      break;
    }
    k >>= 1;
  }
  return(res);
/*    GetMonoImage(8 + ((i%3)*120), 10 + ((i/3)*70), 112, 50);
    GetMonoImage(8 + ((i%3)*120), 10 + ((i/3)*70) + 210, 112, 50);*/
}

#ifdef __WIN32__
char CP866To1251(char c) {
unsigned char b;
  b = (unsigned char) c;
  do {
    if ((b >= 128) && (b <= 175)) { b += 64; break; }
    if ((b >= 224) && (b <= 239)) { b += 16; break; }
    if (b == 240) { b = 168; break; }
    if (b == 241) { b = 184; break; }
  } while (0);
  return((char) b);
}
#endif

void SaveResults(int idx) {
unsigned short i, j, cnt[2];
unsigned char b;
char s[32], *t;
FILE *fl;
  if (idx > list->count) {
    idx = list->count;
  }
  if (idx > 0) {
    fl = fopen(
#ifdef __MSDOS__
      "RESULT-D.TXT",
#endif
#ifdef __WIN32__
      "RESULT-W.TXT",
#endif
      "wb"
    );
    if (fl) {
      strcpy(s, "? | \r\n");
      t = GetStrText(msgs, 3);
      if (t) {
        j = strlen(t);
        while (j--) {
          b = (unsigned char) *t; t++;
          #ifdef __WIN32__
          b = CP866To1251(b);
          #endif
          fwrite(&b, 1, 1, fl);
        }
      }
      for (i = 0; i < 2; i++) {
        fwrite(&s[4], 2, 1, fl);
        cnt[i] = 0;
      }
      for (i = 0; i < list->count; i++) {
        if (i < idx) {
          s[0] = answ[i] ? '+' : '-';
          cnt[(unsigned char) answ[i]]++;
        } else {
          s[0] = ' ';
        }
        fwrite(s, 4, 1, fl);
        t = GET_TEST_NAME_TEXT(list, i);
        j = GET_TEST_NAME_SIZE(list, i) - 1;
        while (j--) {
          b = (unsigned char) *t; t++;
          #ifdef __WIN32__
          b = CP866To1251(b);
          #endif
          fwrite(&b, 1, 1, fl);
        }
        fwrite(&s[4], 2, 1, fl);
      }
      fwrite(&s[4], 2, 1, fl);
      t = GetStrText(msgs, 4);
      if (t) {
        j = strlen(t);
        while (j--) {
          b = (unsigned char) *t; t++;
          #ifdef __WIN32__
          b = CP866To1251(b);
          #endif
          fwrite(&b, 1, 1, fl);
        }
      }
      sprintf(&s[7], " %d(+) %d(-) (%d/%d)", cnt[1], cnt[0], idx, list->count);
      fwrite(&s[7], strlen(&s[7]), 1, fl);
      fwrite(&s[4], 2, 1, fl);
      fclose(fl);
    }
  }
}

int main(void) {
int i, key;
  /* load resources */
  list = GetTestData("TESTINFO");
  msgs = GetFileFromArchive("MESSAGES");
  font = GetFileFromArchive("DOS8X8RU");
  if (list) {
    answ = (char *) malloc(list->count);
  }
  i = 1;
  if (list && msgs && font && answ) {
    i = 2;
    key = InitGraphics();
    if (key) {
      SetActiveFont(font);
      memset(answ, 0, list->count);
      InitScreen();
      i = 0;
      DrawScreen(i);
      do {
        key = GetKeyCode();
        if ((key >= KB_1) && (key <= KB_9)) {
          if (PutTestImage(i, key - KB_1 + 1)) {
            i++;
            if (i < list->count) {
              DrawScreen(i);
            }
          }
        }
      } while ((key != KB_ESC) && (i < list->count));
      FreeGraphics();
      SaveResults(i);
      i = 0;
    }
  }
  /* cleanup */
  if (answ) { free(answ); }
  if (font) { free(font); }
  if (msgs) { free(msgs); }
  if (list) { FreeTestData(list); }
  /* error handling */
  switch (i) {
    case 1: 
      printf("Error: not enough memory or error reading resources from \"TPROGRAM.WDL\" file.\n");
      break;
    case 2:
      printf("Error: VGA adapter with at least 640x480 resolution required.\n");
      break;
  }
  return(i);
}

#ifdef __WIN32__
void mainCRTStartup(void) {
  ExitProcess(main());
}
#endif
