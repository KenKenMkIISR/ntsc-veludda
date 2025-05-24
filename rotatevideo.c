/*----------------------------------------------------------------------------

Copyright (C) 2025, KenKen, all right reserved.

This program supplied herewith by KenKen is free software; you can
redistribute it and/or modify it under the terms of the same license written
here and only for non-commercial purpose.

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of FITNESS FOR A PARTICULAR
PURPOSE. The copyright owner and contributors are NOT LIABLE for any damages
caused by using this program.

----------------------------------------------------------------------------*/

// This signal generation program (using PWM and DMA) is the idea of @lovyan03.
// https://github.com/lovyan03/

//#pragma GCC optimize ("O3")

// ベクトル式VRAM　NTSC出力プログラム　Raspberry Pi Pico用　by KenKen
// 画面上部に固定表示領域あり
//
// VRAM解像度256×256ドット＋最上部固定8行
// 画面出力解像度256×216ドット＋最上部8行
// 256色同時表示、1バイトで1ドットを表す
// カラーパレット対応

// (vstartx,vstarty):画面左上になるVRAM上の座標（256倍）
// (vscanv1_x,vscanv1_y):画面右方向のスキャンベクトル（256倍）
// (vscanv2_x,vscanv2_y):画面下方向のスキャンベクトル（256倍）

#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "rotatevideo.h"

// デバッグ用、割込み処理中HIGHになるピン
//#define PIN_DEBUG_BUSY 0

// グローバル変数定義
unsigned char VRAMALL[VRAM_X*(TOPLINE+VRAM_Y)] __attribute__ ((aligned (4))); //全VRAM領域確保
unsigned char *TOPVRAM=VRAMALL; //画面上部の固定VRAM
unsigned char *VRAM=VRAMALL+VRAM_X*TOPLINE; //回転表示用VRAM
short vscanv1_x,vscanv1_y,vscanv2_x,vscanv2_y;	//映像表示スキャン用ベクトル
short vscanstartx,vscanstarty; //映像表示スキャン開始座標

volatile static uint16_t scanline;
volatile uint16_t drawcount=0; //　1画面表示終了ごとに1足す。アプリ側で0にする。
volatile int8_t drawing; //　映像区間処理中は-1、その他は0
uint8_t videostop; // 1～3：ビデオ出力停止、0：ビデオ出力中

static uint pwm_slice_num; //ビデオ出力ポートのPWM slice number
static uint16_t black_level, white_level;

// DMAピンポンバッファ
uint16_t dma_buffer[2][NUM_LINE_SAMPLES] __attribute__ ((aligned (4)));

// カラーパレット
uint16_t color_tbl[4*256] __attribute__ ((aligned (4)));

static uint pwm_dma_chan0,pwm_dma_chan1;

//画面上部固定領域およびベクトル式VRAM領域をパラメータに従い出力する
static void __not_in_flash_func() makeDmaBuffer(uint16_t* buf, size_t line_num)
{
	static short vscanx,vscany; //映像表示スキャン処理中座標
	static short vscanv1_xx,vscanv1_yy,vscanv2_xx,vscanv2_yy;	//映像表示スキャン用ベクトル(1フレーム描画中固定)
	uint8_t* fbp;
	uint16_t* b = buf;

	if(videostop){
		if(videostop<3){
			drawing=0;
			while (b < buf + NUM_LINE_SAMPLES) *b++ = 0;
			videostop++;
		}
		if(line_num==V_SYNC+V_PREEQ+TOPLINE+DISPYSIZE){
			drawcount++;
		}
		return;
	}
	if (line_num < 2)
	{
		//垂直同期信号生成
		for (int j = 0; j < NUM_LINE_SAMPLES-H_SYNC; j++) *b++ = 0;
		while (b < buf + NUM_LINE_SAMPLES) *b++ = black_level;
	}
	if(line_num==V_SYNC || line_num==V_SYNC+1)
	{
		//水平同期＋バースト信号生成
		for (int j = 0; j < H_SYNC; j++) *b++ = 0;
		for (int j = 0; j < 8; j++) *b++ = black_level;
		for (int j = 0; j < 9; j++)
		{
			*b++=black_level;
			*b++=black_level-1;
			*b++=black_level;
			*b++=black_level+1;
		}
		while (b < buf + NUM_LINE_SAMPLES) *b++ = black_level;
	}
	else if(line_num>=V_SYNC+V_PREEQ && line_num<V_SYNC+V_PREEQ+TOPLINE)
	{
		//映像領域信号生成（固定部）
		b+=H_PICTURE;
		if (line_num == V_SYNC + V_PREEQ)
		{
			drawing = -1;
		}
		fbp = TOPVRAM+(line_num-(V_SYNC + V_PREEQ))*DISPXSIZE;
		for(int i=0;i<DISPXSIZE/2;i++)
		{
			*(uint32_t*)(b)=*(uint32_t*)(color_tbl+(*fbp)*4);
			*(uint32_t*)(b+2)=*(uint32_t*)(color_tbl+(*(fbp+1))*4+2);
			fbp+=2;
			b+=4;
		}
	}
	else if(line_num>=V_SYNC+V_PREEQ+TOPLINE && line_num<V_SYNC+V_PREEQ+TOPLINE+DISPYSIZE)
	{
		if(line_num==V_SYNC+V_PREEQ+TOPLINE)
		{
			vscanx=vscanstartx;
			vscany=vscanstarty;
			vscanv1_xx=vscanv1_x;
			vscanv1_yy=vscanv1_y;
			vscanv2_xx=vscanv2_x;
			vscanv2_yy=vscanv2_y;
		}
		//映像領域信号生成（回転部）
		b+=H_PICTURE;
		fbp = TOPVRAM+(line_num-(V_SYNC + V_PREEQ))*DISPXSIZE;
		uint16_t x=vscanx;
		uint16_t y=vscany;
		for(int i=0;i<DISPXSIZE/2;i++)
		{
			*(uint32_t*)(b)=*(uint32_t*)(color_tbl+VRAM[(y&0xff00)+(x>>8)]*4);
			x+=vscanv1_xx;
			y+=vscanv1_yy;
			*(uint32_t*)(b+2)=*(uint32_t*)(color_tbl+VRAM[(y&0xff00)+(x>>8)]*4+2);
			x+=vscanv1_xx;
			y+=vscanv1_yy;
			b+=4;
		}
		vscanx+=vscanv2_xx;
		vscany+=vscanv2_yy;
	}
	if(line_num==V_SYNC+V_PREEQ+TOPLINE+DISPYSIZE || line_num==V_SYNC+V_PREEQ+TOPLINE+DISPYSIZE+1)
	{
		if(line_num==V_SYNC+V_PREEQ+TOPLINE+DISPYSIZE){
			drawing=0;
			drawcount++;
		}
		//映像領域信号消去
		b+=H_PICTURE;
		for(int i=0;i<DISPXSIZE*2;i++) *b++ = black_level;
	}
}

void set_palette(unsigned char c,unsigned char b,unsigned char r,unsigned char g){
	// カラーパレット設定
	// c:パレット番号0-255、r,g,b:0-255

	// 輝度Y=0.587*G+0.114*B+0.299*R
	// 信号N=Y+0.4921*(B-Y)*sinθ+0.8773*(R-Y)*cosθ
	// 出力データS=N*(white-black)/255+black  white=9,black=2

	int32_t y,s;
	y=(150*g+29*b+77*r+128)/256;

	// 微妙な色ズレを修正
/*
	float th=30.0/180*3.14159;
	int32_t b_y_1=(int32_t)((b-y)*256*7*0.4921*sinf(0+th));
	int32_t r_y_1=(int32_t)((r-y)*256*7*0.8773*cosf(0+th));
	int32_t b_y_2=(int32_t)((b-y)*256*7*0.4921*sinf(3.14159/2+th));
	int32_t r_y_2=(int32_t)((r-y)*256*7*0.8773*cosf(3.14159/2+th));
*/
	int32_t b_y_1=(b-y)*441;
	int32_t r_y_1=(r-y)*1361;
	int32_t b_y_2=(b-y)*764;
	int32_t r_y_2=(r-y)*(-786);

	s=(y*1792 + b_y_1 + r_y_1 + 2*65536+32768)/65536;
	color_tbl[c*4] = s<0 ? 0 : s;
	s=(y*1792 + b_y_2 + r_y_2 + 2*65536+32768)/65536;
	color_tbl[c*4+1] = s<0 ? 0 : s;
	s=(y*1792 - b_y_1 - r_y_1 + 2*65536+32768)/65536;
	color_tbl[c*4+2] = s<0 ? 0 : s;
	s=(y*1792 - b_y_2 - r_y_2 + 2*65536+32768)/65536;
	color_tbl[c*4+3] = s<0 ? 0 : s;
}

void init_palette(void){
	//カラーパレット初期化
	int i;
	for(i=0;i<8;i++){
		set_palette(i,255*(i&1),255*((i>>1)&1),255*(i>>2));
	}
	for(i=0;i<8;i++){
		set_palette(i+8,128*(i&1),128*((i>>1)&1),128*(i>>2));
	}
	for(i=16;i<256;i++){
		set_palette(i,255,255,255);
	}
}

static void irq_handler(void) {
	volatile uint32_t s0;

#if defined ( PIN_DEBUG_BUSY )
	gpio_put(PIN_DEBUG_BUSY, 1);
#endif
	s0=dma_hw->ints0;
	dma_hw->ints0 = s0;
	if(s0 & (1u << pwm_dma_chan1)){
		makeDmaBuffer(dma_buffer[1], scanline);
		dma_channel_set_read_addr(pwm_dma_chan1, dma_buffer[1], false);
	}
	else{
		makeDmaBuffer(dma_buffer[0], scanline);
		dma_channel_set_read_addr(pwm_dma_chan0, dma_buffer[0], false);
	}
	if (++scanline >= NUM_LINES) {
		scanline = 0;
	}
#if defined ( PIN_DEBUG_BUSY )
	gpio_put(PIN_DEBUG_BUSY, 0);
#endif
}

//画面クリア
void clearscreen(void)
{
	unsigned int *vp;
	int i;
	vp=(unsigned int *)VRAM;
	for(i=0;i<VRAM_X*VRAM_Y/4;i++) *vp++=0;
	vp=(unsigned int *)TOPVRAM;
	for(i=0;i<VRAM_X*TOPLINE/4;i++) *vp++=0;
}

void set_dma_channel_chain(uint ch,uint ch_to){
	uint32_t a;
	a=dma_channel_hw_addr(ch)->al1_ctrl;
	a=(a & ~DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS) | (ch_to << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB);
	dma_channel_hw_addr(ch)->al1_ctrl=a;
}
//ビデオ出力開始
void start_composite(void)
{
	scanline=0;
	videostop=0;
}
//ビデオ出力停止
void stop_composite(void)
{
	videostop=1;
}

//ビデオ出力システムを初期化し開始する
//n:信号出力するGPIO番号
void rp2040_pwm_ntsc_init(uint8_t n)
{
#if defined ( PIN_DEBUG_BUSY )
	gpio_init(PIN_DEBUG_BUSY);
	gpio_set_dir(PIN_DEBUG_BUSY, GPIO_OUT);
#endif
	black_level=2;
	white_level=9;

	init_palette();

	//VRAMスキャンベクトル初期化
	vscanv1_x=256;
	vscanv1_y=0;
	vscanv2_x=0;
	vscanv2_y=256;
	vscanstartx=0;
	vscanstarty=0;

	// CPUを157.5MHzで動作させる
	uint32_t freq_khz = 157500;

	// PWM周期を11サイクルとする (157.5 [MHz] / 11 = 14318181 [Hz])
	uint32_t pwm_div = 11;

	// ※ NTSCのカラー信号を1周期4サンプルで出力する。
	// 出力されるカラーバースト信号は  14318181 [Hz] / 4 = 3579545 [Hz] となる。

	set_sys_clock_khz(freq_khz, true);

	gpio_set_function(n, GPIO_FUNC_PWM);
	pwm_slice_num = pwm_gpio_to_slice_num(n);

	pwm_config config = pwm_get_default_config();
	pwm_config_set_clkdiv(&config, 1);

	pwm_init(pwm_slice_num, &config, true);
	pwm_set_wrap(pwm_slice_num, pwm_div - 1);

	volatile void* wr_addr = &pwm_hw->slice[pwm_slice_num].cc;
	wr_addr = (volatile void*)(((uintptr_t)wr_addr) + 2);

	pwm_dma_chan0 = dma_claim_unused_channel(true);
	pwm_dma_chan1 = dma_claim_unused_channel(true);

	dma_channel_config c = dma_channel_get_default_config(pwm_dma_chan0);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
	channel_config_set_read_increment(&c, true);
	channel_config_set_write_increment(&c, false);
	channel_config_set_dreq(&c, DREQ_PWM_WRAP0 + pwm_slice_num);
	channel_config_set_chain_to(&c, pwm_dma_chan1);
	dma_channel_configure(
		pwm_dma_chan0,
		&c,
		wr_addr,
		dma_buffer[0],
		NUM_LINE_SAMPLES,
		false
	);

	c = dma_channel_get_default_config(pwm_dma_chan1);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
	channel_config_set_read_increment(&c, true);
	channel_config_set_write_increment(&c, false);
	channel_config_set_dreq(&c, DREQ_PWM_WRAP0 + pwm_slice_num);
	channel_config_set_chain_to(&c, pwm_dma_chan0);
	dma_channel_configure(
		pwm_dma_chan1,
		&c,
		wr_addr,
		dma_buffer[1],
		NUM_LINE_SAMPLES,
		false
	);
	scanline=0;
	videostop=0;
	makeDmaBuffer(dma_buffer[0], 0);
	makeDmaBuffer(dma_buffer[1], 1);
	dma_set_irq0_channel_mask_enabled((1u<<pwm_dma_chan0)|(1u<<pwm_dma_chan1),true);
	irq_set_exclusive_handler(DMA_IRQ_0, irq_handler);
	irq_set_enabled(DMA_IRQ_0, true);
	dma_channel_start(pwm_dma_chan0);
}
