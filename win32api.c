#ifdef __WIN32__
/*
  CRT function to WinAPI
  do not use msvcrt.dll - only Windows system libs
*/

#include "win32api.h"

void *memset(void *s, int c, size_t n) {
char *ptr;
  for (ptr = s; n > 0; n--) {
    *(ptr++) = (char) c;
  }
  return(s);
}

void *memmove(void *dst, const void *src, size_t count) {
void *ret;
  ret = dst;
  if ((dst <= src) || ((char *)dst >= ((char *)src + count))) {
    while (count--) {
      *(char *)dst = *(char *)src;
      dst = (char *)dst + 1;
      src = (char *)src + 1;
    }
  } else {
    dst = (char *)dst + count - 1;
    src = (char *)src + count - 1;
    while (count--) {
      *(char *)dst = *(char *)src;
      dst = (char *)dst - 1;
      src = (char *)src - 1;
    }
  }
  return(ret);
}

#define FILE2HANDLE(x) ((HANDLE)((x) ? (x) : INVALID_HANDLE_VALUE))

FILE *fopen(const char *filename, const char *mode) {
DWORD fxmode, fxopen;
HANDLE fl;
  fxmode = 0;
  fxopen = OPEN_EXISTING;
  /* byte/text modes not handled */
  while (*mode) {
    switch (*mode) {
      case 'r':
        fxmode |= GENERIC_READ;
        break;
      case '+':
        fxmode |= GENERIC_WRITE;
        break;
      case 'w':
        fxopen = CREATE_ALWAYS;
        fxmode |= GENERIC_WRITE;
        break;
    }
    mode++;
  }
  fl = CreateFile(filename, fxmode, 0, NULL, fxopen, (fxmode | GENERIC_READ) ? FILE_FLAG_SEQUENTIAL_SCAN : 0, 0);
  if (fl == INVALID_HANDLE_VALUE) { fl = 0; }
  return((FILE *) fl);
}

size_t fread(void *ptr, size_t size, size_t n, FILE *stream) {
DWORD dw;
  dw = 0;
  ReadFile(FILE2HANDLE(stream), ptr, size * n, &dw, NULL);
  return(dw);
}

size_t fwrite(const void *ptr, size_t size, size_t n, FILE*stream) {
DWORD dw;
  dw = 0;
  WriteFile(FILE2HANDLE(stream), ptr, size * n, &dw, NULL);
  return(dw);
}

int fclose(FILE *stream) {
  return(CloseHandle(FILE2HANDLE(stream)) ? 0 : -1);
}

int fseek(FILE *stream, long offset, int whence) {
  SetFilePointer(FILE2HANDLE(stream), offset, NULL, whence);
  return(stream ? 0 : 1);
}

long ftell(FILE *stream) {
  return(stream ? SetFilePointer(FILE2HANDLE(stream), 0, NULL, FILE_CURRENT) : -1);
}

#endif
