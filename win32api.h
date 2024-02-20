#ifdef __WIN32__

#ifndef __WIN32API_H
#define __WIN32API_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define sprintf wsprintf
#define strncpy lstrcpyn
#define malloc(x) ((void *) LocalAlloc(LPTR, (x)))
#define free(x) LocalFree((x))
#define printf(x) MessageBox(0, (x), NULL, MB_OK | MB_ICONERROR | MB_TASKMODAL)

void *memset(void *s, int c, size_t n);
FILE *fopen(const char *filename, const char *mode);
size_t fread(void *ptr, size_t size, size_t n, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t n, FILE*stream);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);

#endif

#endif
