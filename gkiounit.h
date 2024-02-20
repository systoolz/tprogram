#ifndef __GKIOUNIT_H
#define __GKIOUNIT_H

#define VID_MAX_X 639
#define VID_MAX_Y 479

/* dropped CONIO.H - saved about 2 Kb! */
/* keyboard scancodes */
#define KB_ESC    0x01
#define KB_ENTER  0x1C
#define KB_1      0x02
#define KB_9      0x0A

unsigned int x_kbhit(void);
unsigned int x_getch(void);
void ClearScreen(void);
int InitGraphics(void);
void FreeGraphics(void);
void DrawLineX(int i1, int j1, int i2, int j2);
void DrawTextX(int x, int y, char *s, int len);
void DrawRectX(int x1, int y1, int x2, int y2);
void FillRectX(int x1, int y1, int x2, int y2);
void SetGraphColor(unsigned char color);
void SetActiveFont(unsigned char *font);
void PutMonoImage(unsigned int x, unsigned int y, unsigned char *p);
unsigned char *GetMonoImage(unsigned int x, unsigned int y, unsigned int w, unsigned int h);

#endif
