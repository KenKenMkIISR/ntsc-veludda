// ベクトル式VRAM　ビデオ出力プログラム　ヘッダーファイル　by KenKen
// 画面上部は固定VRAM
#include <stdint.h>

#define VRAM_X   256 // VRAMの横方向最大値
#define VRAM_Y   256 // VRAMの縦方向最大値
#define TOPLINE 8 // 画面上部の固定表示行数
#define DISPXSIZE 256 // ベクトル式VRAMの出力部分の横ピクセル数
#define DISPYSIZE 216 // ベクトル式VRAMの出力部分の縦ピクセル数

// NTSC出力 走査線数
#define NUM_LINES 262  // 走査線262本
#define V_SYNC		10	// 垂直同期本数
#define V_PREEQ		22	// ブランキング区間上側
#define H_SYNC		68	// 水平同期幅、約4.7μsec
#define H_PICTURE (H_SYNC+8+9*4+120) // 映像開始位置
#define init_composite rp2040_pwm_ntsc_init

// NTSC出力 1ラインあたりのサンプル数
#define NUM_LINE_SAMPLES 908  // 227 * 4

void clearscreen(void);
void start_composite(void);
void stop_composite(void);
void rp2040_pwm_ntsc_init(uint8_t n);
void init_palette(void);
void set_palette(unsigned char c,unsigned char b,unsigned char r,unsigned char g);

extern volatile int8_t drawing; //　映像区間処理中は-1、その他は0
extern volatile unsigned short drawcount;		//　1画面表示終了ごとに1足す。アプリ側で0にする。
							// 最低1回は画面表示したことのチェックと、アプリの処理が何画面期間必要かの確認に利用。
extern unsigned char *VRAM; // ビデオメモリ
extern unsigned char *TOPVRAM; // 画面上部の固定表示用ビデオメモリ

// (vstartx,vstarty):画面左上になるVRAM上の座標（256倍）
// (vscanv1_x,vscanv1_y):画面右方向のスキャンベクトル（256倍）
// (vscanv2_x,vscanv2_y):画面下方向のスキャンベクトル（256倍）
extern short vscanv1_x,vscanv1_y,vscanv2_x,vscanv2_y;	//映像表示スキャン用ベクトル
extern short vscanstartx,vscanstarty; //映像表示スキャン開始座標

