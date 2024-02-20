#ifndef __TESTUNIT_H
#define __TESTUNIT_H

#pragma pack(push, 1)
typedef struct {
  unsigned long mark;
  unsigned long much;
  unsigned long offs;
} packhead;

typedef struct {
  unsigned long offs;
  unsigned short pack;
  unsigned short size;
  char name[8];
} packlist;
#pragma pack(pop)

typedef struct {
  char *file;
  char *name;
  unsigned char data[4];
} testlist;

typedef struct {
  unsigned short count;
  unsigned char *data;
  testlist *test;
} testdata;

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

unsigned char *GetFileFromArchive(char *filename);
testdata *GetTestData(char *filename);
void FreeTestData(testdata *p);
char *GetStrText(unsigned char *data, unsigned int idx);

#define GET_TEST_NAME_FILE(x, y) ((x)->test[(y)].file)
#define GET_TEST_NAME_TEXT(x, y) ((x)->test[(y)].name)
#define GET_TEST_RIGHT_NUM(x, y) ((x)->test[(y)].data[0])
#define GET_TEST_NAME_SIZE(x, y) ((x)->test[(y)].data[1])
#define GET_TEST_BAR_PLACE(x, y) ((x)->test[(y)].data[3] >> 4)
#define GET_TEST_IMG_ORDER(x, y) ((((x)->test[(y)].data[3] << 8) | (x)->test[(y)].data[2]) & 0x1FF)

#endif
