// グラフィックライブラリ用ヘッダーファイル
// 画面上部の固定表示領域対応

extern const unsigned char FontData[];
void pset(int x,int y,unsigned char c);
void pset_fixarea(int x,int y,unsigned char c);
void putbmpmn(int x,int y,unsigned char m,unsigned char n,const unsigned char bmp[]);
void clrbmpmn(int x,int y,unsigned char m,unsigned char n);
void putfont(int x,int y,unsigned int c,unsigned char n);
void putfont_fixarea(int x,int y,unsigned int c,unsigned char n);
void line(int x1,int y1,int x2,int y2,unsigned int c);
void circle(int x0,int y0,unsigned int r,unsigned int c);
void printstr(int x,int y,unsigned int c,unsigned char *s);
void printstr_fixarea(int x,int y,unsigned int c,unsigned char *s);
void printnum(int x,int y,unsigned char c,unsigned int n);
void printnum_fixarea(int x,int y,unsigned char c,unsigned int n);
void initvscanv();
void pset2(unsigned char x,unsigned char y,unsigned char c);
void putfont2(int x,int y,unsigned int c,unsigned char n);
void printstr2(int x,int y,unsigned int c,unsigned char *s);
void printnum2(int x,int y,unsigned char c,unsigned int n);
void putbmpmn_fixarea(unsigned char x,unsigned char y,unsigned char m,unsigned char n,const unsigned char bmp[]);
void clrbmpmn_fixarea(unsigned char x,unsigned char y,unsigned char m,unsigned char n);
void putbmpmn2(unsigned char x,unsigned char y,unsigned char m,unsigned char n,unsigned char bmp[]);
void putbmpmn3(unsigned char x,unsigned char y,unsigned char m,unsigned char n,const unsigned char bmp[]);
