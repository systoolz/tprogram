#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __TURBOC__

#include <alloc.h>
#include <dir.h>
#include <dos.h>

#define INVALID_HANDLE_VALUE (-1)
#define DWORD unsigned long
#define WORD unsigned short
#define BYTE unsigned char
#define HANDLE int

#define LOWORD(x) ((x) & 0xFFFF)
#define MAKELONG(x, y) ((((DWORD)(y)) << 16) | LOWORD(x))

#define WIN32_FIND_DATA struct ffblk
#define cFileName ff_name
#define nFileSizeLow ff_fsize

#define FindFirstFile(x, y) findfirst(x, y, FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_ARCH)
#define FindNextFile(x, y) (!findnext(y))
#define FindClose(h) h++

#else

#include <malloc.h>
#include <windows.h>

#endif

#pragma pack(push, 1)
typedef struct {
  char magic[8];
  char compType;
  char lastChar;
  DWORD UPackSize;
} lzsshead;

typedef struct {
  DWORD mark;
  DWORD much;
  DWORD offs;
} wad_head;

typedef struct {
  DWORD offs;
  WORD pack;
  WORD size;
  char name[8];
} wad_pack;
#pragma pack(pop)

int main(int argc, char *argv[]) {
WIN32_FIND_DATA sd;
HANDLE sh;
wad_pack *pack;
wad_head head;
lzsshead flhd;
FILE *fl, *fin;
DWORD sz;
BYTE *p;
int i, j;

  if (argc <= 1) {
    printf("Error: filemask not specified!\n");
    return(1);
  }

  head.mark = 0x5A4C4457UL;
  head.offs = sizeof(head);
  head.much = 0;

  printf("packing...");
  fl = NULL;
  pack = NULL;
  for (i = 1; i < argc; i++) {
    memset(&sd, 0, sizeof(sd));
    sh = FindFirstFile(argv[i], &sd);
    if (sh != INVALID_HANDLE_VALUE) {
      do {
        if (strlen(sd.cFileName) > 12) {
          printf(" (LONG_FILE_NAME: %s) ", sd.cFileName);
          break;
        }
        if (!fl) {
          fl = fopen("TPROGRAM.WDL", "wb");
          if (!fl) { break; }
          fwrite(&head, 1, sizeof(head), fl);
        }
        fin = fopen(sd.cFileName, "rb");
        /* get file size */
        fseek(fin, 0, SEEK_END);
        sz = ftell(fin) - sizeof(flhd);
        fseek(fin, 0, SEEK_SET);
        /* form packed item name */
        sd.cFileName[8] = 0;
        for (j = 0; j < 8; j++) {
          if (sd.cFileName[j] == '.') {
            memset(&sd.cFileName[j], 0, 8 - j);
            break;
          }
          sd.cFileName[j] -= ((sd.cFileName[j] >= 'a') && (sd.cFileName[j] <= 'z')) ? ('a' - 'A') : 0;
        }
        printf("%s", sd.cFileName);
        /* read header */
        memset(&flhd, 0, sizeof(flhd));
        fread(&flhd, sizeof(flhd), 1, fin);
        /* check header */
        if (strncmp(flhd.magic, "SZDD\x88\xF0\x27\x33""A", 9)) {
          fseek(fin, 0, SEEK_SET);
          flhd.UPackSize = 0;
          sz = sd.nFileSizeLow;
        }
        pack = (wad_pack *) realloc(pack, ((unsigned int) head.much + 1) * sizeof(pack[0]));
        memset(&pack[(unsigned int) head.much], 0, sizeof(pack[0]));
        memcpy(pack[(unsigned int) head.much].name, sd.cFileName, 8);

        if (head.much) {
          pack[(unsigned int) head.much].offs =
            pack[(unsigned int) head.much - 1].offs +
            LOWORD(pack[(unsigned int) head.much - 1].pack);
        } else {
          pack[(unsigned int) head.much].offs = sizeof(head);
        }

        p = (BYTE *) malloc((unsigned int) sz);
        fread(p, (unsigned int) sz, 1, fin);
        fwrite(p, (unsigned int) sz, 1, fl);
        free(p);

        pack[(unsigned int) head.much].pack = sz;
        pack[(unsigned int) head.much].size = flhd.UPackSize;
        head.offs += sz;

        sz = strlen(sd.cFileName);
        while (sz > 0) {
          printf("\x08\x20\x08");
          sz--;
        }
        head.much++;
        fclose(fin);
      } while(FindNextFile(sh, &sd));
      FindClose(sh);
    }
  }

  if (fl) {
    fwrite(pack, sizeof(pack[0]), (unsigned int) head.much, fl);
    fseek(fl, 0, SEEK_SET);
    fwrite(&head, sizeof(head), 1, fl);
    fclose(fl);
    printf("done\n");
  } else {
    printf("no files\n");
  }

  if (pack) {
    free(pack);
  }

  return(0);
}
