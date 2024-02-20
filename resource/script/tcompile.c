#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __MSDOS__
#include <alloc.h>
#endif

/*
  TESTINFO file format
WORD - tests count
  // for each test
  BYTE - right answer
  BYTE - test name length include tail zero byte (len)
  WORD - flags: (flags & 0x1FF) - answer images placement (bitmask); (flags>>12) - missing image placement
  CHAR[8] - test filename
  CHAR[len] - test name (ASCIIZ string)
*/

char *LoadTextFile(char *filename) {
FILE *fl;
char *res;
long sz;
  res = NULL;
  fl = fopen(filename, "rb");
  if (fl) {
    fseek(fl, 0, SEEK_END);
    sz = ftell(fl);
    fseek(fl, 0, SEEK_SET);
    res = (char *) malloc((unsigned int) sz + 1);
    if (res) {
      fread(res, (unsigned int) sz, 1, fl);
      res[(unsigned int) sz] = 0;
    }
    fclose(fl);
  }
  fclose(fl);
  return(res);
}

int main(void) {
unsigned int x, i, w1, w2;
char *s, *t, *h;
FILE *fl;
  s = LoadTextFile("TESTINFO.TXT");
  if (!s) {
    printf("ERROR: can not open TESTINFO.TXT for read!\n");
  } else {
    fl = fopen("TESTINFO.BIN", "wb");
    if (!fl) {
      printf("ERROR: can not open TESTINFO.BIN for write!");
      free(s);
      return(1);
    }
    x = 0;
    fwrite(&x, 2, 1, fl);
    t = s;
    while (*t) {
      if (*t != ' ') {
        if (*t == ';') {
          while (*t && (*t != 13) && (*t != 10)) {
            t++;
          }
          while ((*t == 13) || (*t == 10)) {
            t++;
          }
        } else {
          h = t;
          while (*t && (*t != 13) && (*t != 10)) {
            t++;
          }
          if (*t) {
            *t = 0;
            t++;
            while ((*t == 13) || (*t == 10)) {
              *t = 0;
              t++;
            }
          }
          if ((strlen(h) >= 24) && (h[1] == h[3]) && (h[3] == h[13]) && (h[13] == h[22]) && (h[22] == '|')) {
            x++;
            printf("%02d %s", x, h);

            w1  = (atoi(h) & 0xF) << 8;
            for (; *h != '|'; h++); h++;

            w1 |= (atoi(h) & 0xF);
            for (; *h != '|'; h++); h++;

            w2 = 0;
            for (i = 0; i < 9; i++) {
              if (*h != '0') {
                w2 |= (1 << i);
              }
              h++;
            }
            h++;

            w2 |= (w1 & 0xF00) << 4;
            w1 &= 0xFF;
            w1 |= ((strlen(&h[9]) + 1) & 0xFF) << 8;
            h[8] = 0;
            for (i = 0; i < 8; i++) {
              if (h[i] == ' ') { h[i] = 0; }
            }
            fwrite(&w1, 1, 2, fl);
            fwrite(&w2, 1, 2, fl);
            fwrite(  h, 1, 8, fl);
            h += 9;
            fwrite(h, 1, strlen(h) + 1, fl);
            printf(" - done\n");
          } else {
            printf("?? %s\n", h);
          }
        }
      } else {
        t++;
      }
    }
    fseek(fl, 0, SEEK_SET);
    fwrite(&x, 2, 1, fl);
    fclose(fl);
    free(s);
    printf("Total: %d\n", x);
  }

  s = LoadTextFile("MESSAGES.TXT");
  if (!s) {
    printf("ERROR: can not open MESSAGES.TXT for read!\n");
  } else {
    x = 0;
    fl = fopen("MESSAGES.BIN", "wb");
    if (!fl) {
      printf("ERROR: can not open MESSAGES.BIN for write!");
      free(s);
      return(1);
    }
    fwrite(&x, 1, 2, fl);
    h = s;
    w2 = strlen(s);
    for (i = 0; i <= w2; i++) {
      if ((s[i] == 13) || (s[i] == 10) || (s[i] == 0)) {
        s[i] = 0;
        w1 = strlen(h);
        if (w1) {
          w1++;
          fwrite(&w1, 1, 2, fl);
          fwrite(h, 1, w1, fl);
        }
        h = &s[i];
        h++;
        x++;
      }
      if (s[i] == '|') { s[i] = 10; }
    }
    fseek(fl, 0, SEEK_SET);
    fwrite(&x, 1, 2, fl);
    fclose(fl);
    free(s);
    printf("Strings: %d\n", x);
  }

  s = LoadTextFile("TESTTEXT.TXT");
  if (!s) {
    printf("ERROR: can not open TESTTEXT.TXT for read!\n");
  } else {
    x = 0;
    fl = fopen("TESTTEXT.BIN", "wb");
    if (!fl) {
      printf("ERROR: can not open TESTTEXT.BIN for write!");
      free(s);
      return(1);
    }
    x = 0;
    for (i = 0; i <= strlen(s); i++) {
      if ((s[i] == 13) || (!s[i])) {
        fwrite(&s[x], 1, i - x, fl);
        x = i + 1;
      }
    }
    i = 0;
    fwrite(&i, 1, 1, fl);
    free(s);
    printf("Title screen done.\n");
  }

  return(0);
}
