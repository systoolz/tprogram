#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __MSDOS__
#include <alloc.h>
#endif

#ifdef __WIN32__
#include "win32api.h"
#endif

#include "testunit.h"

#define BUFFSIZE 0xFFF
#define LZSS_GETBYTE(p, sz, cr) ((cr < sz) ? p[cr++] : -1)
#define LZSS_PUTBYTE(u, sz, cr, b) ((cr < sz) ? (u[cr++] = b) | 0xff : 0)

void lzss_unpack(unsigned char *p, unsigned char *u, unsigned int ps, unsigned int us) {
unsigned char buff[BUFFSIZE + 1];
unsigned int pi, ui;
int i, j, len, m, b;
  memset(buff, 32, BUFFSIZE + 1);
  i = (BUFFSIZE + 1) - 16;
  pi = 0;
  ui = 0;
  while ((pi < ps) && (ui < us)) {
    b = LZSS_GETBYTE(p, ps, pi);
    if (b == -1) { break; }
    for (m = 0; m < 8; m++) {
      if (!(b & 1)) {
        j = LZSS_GETBYTE(p, ps, pi);
        if (j == -1) { break; }
        len = LZSS_GETBYTE(p, ps, pi);
        if (len == -1) { break; }
        j += ((len & 0xF0) << 4);
        len = (len & 0x0F) + 3;
        while (len > 0) {
          buff[i] = buff[j];
          if (!LZSS_PUTBYTE(u, us, ui, buff[i])) { break; }
          j = (j + 1) & BUFFSIZE;
          i = (i + 1) & BUFFSIZE;
          len--;
        }
      } else {
        j = LZSS_GETBYTE(p, ps, pi);
        if (j == -1) { break; }
        buff[i] = j;
        if (!LZSS_PUTBYTE(u, us, ui, buff[i])) { break; }
        i = (i + 1) & BUFFSIZE;
      }
      b >>= 1;
    }
  }
}

char testname(char *s, char *d, unsigned int l) {
char a, b;
  a = 0;
  b = 0;
  while (l--) {
    a = *s;
    b = *d;
    a += ((a >= 'A') && (a <= 'Z')) ? ('a' - 'A') : 0;
    b += ((b >= 'A') && (b <= 'Z')) ? ('a' - 'A') : 0;
    if ((a != b) || (!a) || (!b)) { break; }
    s++;
    d++;
  }
  return(a - b);
}

unsigned char *GetFileFromArchive(char *filename) {
unsigned short i, ps, us;
unsigned char *p, *u;
packhead head;
packlist *list;
FILE *fl;
  p = NULL;
  fl = fopen("TPROGRAM.WDL", "rb");
  if (fl) {
    memset(&head, 0, sizeof(head));
    fread(&head, sizeof(head), 1, fl);
    if (head.mark == 0x5A4C4457UL) {
      list = (packlist *) malloc(((unsigned short) head.much) * sizeof(list[0]));
      if (list) {
        fseek(fl, head.offs, SEEK_SET);
        fread(list, (unsigned short) head.much, sizeof(list[0]), fl);
        for (i = 0; i < (unsigned short) head.much; i++) {
          if (!testname(list[i].name, filename, 8)) {
            fseek(fl, list[i].offs, SEEK_SET);
            ps = list[i].pack;
            us = list[i].size;
            free(list);
            list = NULL;
            u = NULL;
            if (us) {
              u = (unsigned char *) malloc(us);
              if (!u) { break; }
            }
            p = (unsigned char *) malloc(ps);
            if (!p) {
              if (u) { free(u); }
              break;
            }
            fread(p, ps, 1, fl);
            if (u) {
              lzss_unpack(p, u, ps, us);
              free(p);
              p = u;
            }
            break;
          }
        }
      }
      if (list) { free(list); }
    }
    fclose(fl);
  }
  return(p);
}

testdata *GetTestData(char *filename) {
unsigned char *x;
testdata *p;
int i;
  p = (testdata *) malloc(sizeof(p[0]));
  if (p) {
    p->test = NULL;
    p->data = GetFileFromArchive(filename);
    if (p->data) {
      p->count = p->data[0] | (p->data[1] << 8);
      p->test = (testlist *) malloc(p->count * sizeof(p->test[0]));
      if (p->test) {
        x = p->data;
        x += 2;
        for (i = 0; i < p->count; i++) {
          memmove(p->test[i].data, x, 4);
          x += 4;
          p->test[i].file = (char *) x;
          x += 8;
          p->test[i].name = (char *) x;
          x += p->test[i].data[1];
        }
      }
    }
    if (!p->test) {
      if (p->data) {
        free(p->data);
      }
      free(p);
      p = NULL;
    }
  }
  return(p);
}

void FreeTestData(testdata *p) {
  if (p) {
    if (p->test) { free(p->test); }
    if (p->data) { free(p->data); }
    free(p);
  }
}

char *GetStrText(unsigned char *data, unsigned int idx) {
unsigned int i, c;
char *res;
  res = NULL;
  if (data) {
    c = (data[1] << 8) | data[0];
    data += 2;
    for (i = 0; i < c; i++) {
      if (i == idx) {
        res = (char *) &data[2];
        break;
      } else {
        data += ((data[1] << 8) | data[0]) + 2;
      }
    }
  }
  return(res);
}
