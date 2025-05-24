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

// 画面回転スクロールゲーム　VELUDDA for Raspberry Pi Pico by KenKen
// 背景解像度　BMPファイルロード対応
//
// 拡大縮小回転機能付き液晶出力システム使用
// 解像度 横256×縦216ドット＋上8行
// VRAM容量256×256ドット、8ビットカラー

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "rotatevideo.h"
#include "graphlib.h"
#include "veludda.h"
#include "ff.h"
#include "usbkeyboard.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

FATFS FatFs;
unsigned char mapdataram[MAPBMPDX*MAPBMPDY];


//三角関数テーブル（1周256分割、値は256倍）
const short sindata[256]={
	0,6,13,19,25,31,38,44,50,56,62,68,74,80,86,92,
	98,104,109,115,121,126,132,137,142,147,152,157,162,167,172,177,
	181,185,190,194,198,202,206,209,213,216,220,223,226,229,231,234,
	237,239,241,243,245,247,248,250,251,252,253,254,255,255,256,256,
	256,256,256,255,255,254,253,252,251,250,248,247,245,243,241,239,
	237,234,231,229,226,223,220,216,213,209,206,202,198,194,190,185,
	181,177,172,167,162,157,152,147,142,137,132,126,121,115,109,104,
	98,92,86,80,74,68,62,56,50,44,38,31,25,19,13,6,
	0,-6,-13,-19,-25,-31,-38,-44,-50,-56,-62,-68,-74,-80,-86,-92,
	-98,-104,-109,-115,-121,-126,-132,-137,-142,-147,-152,-157,-162,-167,-172,-177,
	-181,-185,-190,-194,-198,-202,-206,-209,-213,-216,-220,-223,-226,-229,-231,-234,
	-237,-239,-241,-243,-245,-247,-248,-250,-251,-252,-253,-254,-255,-255,-256,-256,
	-256,-256,-256,-255,-255,-254,-253,-252,-251,-250,-248,-247,-245,-243,-241,-239,
	-237,-234,-231,-229,-226,-223,-220,-216,-213,-209,-206,-202,-198,-194,-190,-185,
	-181,-177,-172,-167,-162,-158,-153,-147,-142,-137,-132,-126,-121,-115,-109,-104,
	-98,-92,-86,-80,-74,-68,-62,-56,-50,-44,-38,-31,-25,-19,-13,-6
};
// 逆正接データ
// x>=yの時の、y*256/xの角度（45度まで）
const unsigned char atandata[257]={
	 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5,
	 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9,10,10,10,
	10,10,10,10,11,11,11,11,11,11,11,12,12,12,12,12,12,12,13,13,13,13,13,13,13,14,14,14,14,14,14,14,
	15,15,15,15,15,15,15,16,16,16,16,16,16,16,17,17,17,17,17,17,17,17,18,18,18,18,18,18,18,19,19,19,
	19,19,19,19,19,20,20,20,20,20,20,20,20,21,21,21,21,21,21,21,21,21,22,22,22,22,22,22,22,22,23,23,
	23,23,23,23,23,23,23,24,24,24,24,24,24,24,24,24,25,25,25,25,25,25,25,25,25,25,26,26,26,26,26,26,
	26,26,26,27,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,29,29,29,29,29,29,29,29,
	29,29,29,30,30,30,30,30,30,30,30,30,30,30,31,31,31,31,31,31,31,31,31,31,31,31,32,32,32,32,32,32,
	32
};

//敵の隊列を設定
//繰り返し数,敵番号,カウンタ,画像,X,Y,VX,VY,DX,DY
const short enemyposition[][10]={

	//ステージ1用
	{5,0,0,2, 452,200,   0, 256, 15, 15},//0
	{4,0,0,2, 527,245,   0, 256, 15,-15},//1
	{5,0,0,0, 964,320,   0,-256, 15,-15},//2
	{4,0,0,0,  15,275,   0,-256, 15, 15},//3
	{5,0,0,1, 452, 20, 256,   0, 15, 15},//4
	{4,0,0,1, 527, 95, 256,   0,-15, 15},//5
	{5,0,0,3,  60,340,-256,   0,-15, 15},//6
	{4,0,0,3,  15,415,-256,   0, 15, 15},//7

	//ステージ2用
	{5,1,0,0, 500,250,   0, 256,  0, 20},//8
	{5,1,0,0, 524,250,   0, 256,  0, 20},//9
	{5,1,0,0, 200,100, 181, 181,-15,-15},//10
	{5,1,0,0, 950, 50,-181,-181,-15,-15},//11
	{5,1,0,0,  50,450, 256,   0, 20,  0},//12
	{5,1,0,0,  50,474, 256,   0, 20,  0},//13

	//ステージ3用
	{6,2,-1,0, 50,250, 181, 181,250, 50},//14
	{6,2,-1,1,150,120,-181, 181,280, 30},//15
	{6,2,-1,2,130, 80,-181,-181,300,100},//16
	{6,2,-1,3,100, 50, 181,-181,250, 60},//17

	//ステージ4用
	{6,3,-1,0, 50,250, 181, 181,250, 50},//18
	{6,3,-1,0,150,120,-181, 181,280, 30},//19
	{6,3,-1,0,130, 80,-181,-181,300,100},//20
	{6,3,-1,0,100, 50, 181,-181,250, 60},//21

	//ステージ5用
	{6,4,-1,0, 50, 50,   0,   0,180, 30},//22
	{6,4,-1,0, 50,170,   0,   0,180, 30},//23
	{6,4,-1,0, 50,290,   0,   0,180, 30},//24
	{6,4,-1,0, 50,410,   0,   0,180, 30},//25

	//ステージ6用
	{6,5,-1,0, 50, 50,   0,   0,180, 30},//26
	{6,5,-1,0, 50,170,   0,   0,180, 30},//27
	{6,5,-1,0, 50,290,   0,   0,180, 30},//28
	{6,5,-1,0, 50,410,   0,   0,180, 30} //29
};

//地上敵の位置
//繰り返し数,X,Y,DX,DY
const short gndenemyposition[][5]={
	{5, 20,140, 80,  0},//0
	{2,320, 40,  0,150},//1
	{4,620, 10, 45,  0},//2
	{2,560, 60, 20,  0},//3
	{2,560, 80, 20,  0},//4
	{3,280,350, 70,  0},//5
	{2,180,440,110,  0},//6
	{5,705,220, 60,  0},//7
	{4,660,300,  0, 55},//8
	{4,990,300,  0, 55} //9
};

//各ステージのenemyposition,gndenemyposition配列番号を列挙
//最初に空中敵、255から後ろは地上敵、最後に255で1ステージ分終了
const unsigned char enemytable[][20]={
	{0,1,2,3,4,5,6,7,255,255},
	{8,9,10,11,12,13,255,0,5,9,255},
	{14,15,16,17,255,0,1,3,4,5,8,255},
	{18,19,20,21,255,6,255},
	{22,23,24,25,255,0,3,4,2,7,5,6,255},
	{26,27,28,29,255,0,1,2,7,9,5,255},
	{2,3,4,5,8,9,12,13,14,255,0,8,2,5,9,255},
	{0,1,18,19,22,25,28,255,1,3,4,5,9,255},
	{6,7,10,11,17,20,24,26,255,0,2,5,7,8,9,255}
};

#define GNDENEMYSCORE 500 //地上敵の得点

//空中敵の得点テーブル
const unsigned short scoretable[]={
	10,20,50,80,40,30
};

//敵キャラクター名前
unsigned char *enemyname[]={
	"STRA",
	"KURURU",
	"ZIGZA",
	"FIRET",
	"BYON",
	"ROUNDA"
};

//曲演奏用構造体
struct {
	const unsigned char *p; //曲配列の演奏中の位置
	const unsigned char *startp; //曲配列のリピート位置
	unsigned char count; //発音中の音カウンタ
	unsigned short pr; //発音中の音（タイマ周期）
	unsigned char stop; //0:演奏中、1:終了
}  music; //演奏中の音楽構造体

//曲演奏順テーブル
const unsigned char *musictable[]={
	musicdata2,musicdata3,musicdata4
};

// 三角関数マクロ定義
#define Sin(x) sindata[(x)]
#define Cos(x) sindata[((x)+64)&0xff]

// グローバル変数定義
unsigned char gamestatus;//0:ゲーム開始、1:ステージ数表示中、2:ゲーム中、3:プレイヤー1減、4:ステージクリア、5:ゲームオーバー表示、6:終了
unsigned short gamestatuscount;//ゲームステータスカウンタ
unsigned short gcount; //全体カウンタ
unsigned char stage; //ステージ
unsigned char enemyleft; //敵残数
unsigned char ships; //自機の残数
unsigned int score,highscore; //スコア、ハイスコア
unsigned int keystatus=0,keystatus2,oldkey; //最新のボタン状態と前回のボタン状態
unsigned char random8; //8bit乱数値

//画像退避用バッファ
// ビットマップデータ,VRAM座標x,y,横幅,縦幅の順に保存
unsigned char bmpbuf[6000];
unsigned char *bmpbufp; //bmpbufの先頭位置ポインタ

int r1x,r1y; //画面左上の絶対座標*256
int win_sx,win_sy,win_ex,win_ey; //画面描画されている窓のマップ上の絶対座標の最小、最大
unsigned char scr_sx,scr_sy,scr_ex,scr_ey; //画面描画されている窓のVRAM上の左端、上端、右端、下端の列、行
const unsigned short *sounddatap; //効果音配列の位置、演奏中の音楽よりこちらを優先
unsigned short sound2pr; //砲弾飛来音
unsigned char sound2count;//砲弾飛来音カウンタ
unsigned char music_on;//BGMオンオフ
unsigned char continuecount,continueflag;//コンティニュー情報

struct {
	char on;//0:死亡、1:通常、2:爆発中
	int x,y;//自機の絶対座標*256
	unsigned char angle;//自機の角度
	unsigned char count;//カウンタ
} ship; //自機情報

_Enemy enemybuf[MAX_ENEMY],gndenemybuf[MAX_GNDENEMY];//空中敵、地上敵格納配列
_Missile missilebuf[MAX_MISSILE],cannonbuf[MAX_CANNON];//ミサイル、砲弾格納配列

//音声出力用PWM設定値
#define PWM_WRAP1 845 // 157.5MHz/2.983MHz*16
#define PWM_WRAP2 3379 // 157.5MHz/2.983MHz*16*4
#define PWM_WRAP3 13517 // 157.5MHz/2.983MHz*16*16
uint pwm_slice_num;
uint16_t pwm_wrap=0;
uint16_t pwm_freq=0;

//----------------------
// ここからプログラム開始
//----------------------

// 音声出力
void sound_on(uint16_t f){
	if(f==pwm_freq) return;
	pwm_freq=f;

	if(f==0){
		// sound off
		pwm_set_enabled(pwm_slice_num, false);
		return;
	}
	//周波数によりPWM_WRAPの設定を変更
	if(f<0x1000){
		if(pwm_wrap!=PWM_WRAP1){
			pwm_wrap=PWM_WRAP1;
			pwm_set_wrap(pwm_slice_num, PWM_WRAP1-1);
			pwm_set_chan_level(pwm_slice_num, PWM_CHAN_A, PWM_WRAP1/2); //duti ratio 50%
		}
	}
	else if(f<0x4000){
		if(pwm_wrap!=PWM_WRAP2){
			pwm_wrap=PWM_WRAP2;
			pwm_set_wrap(pwm_slice_num, PWM_WRAP2-1);
			pwm_set_chan_level(pwm_slice_num, PWM_CHAN_A, PWM_WRAP2/2); //duti ratio 50%
		}
		f>>=2;
	}
	else{
		if(pwm_wrap!=PWM_WRAP3){
			pwm_wrap=PWM_WRAP3;
			pwm_set_wrap(pwm_slice_num, PWM_WRAP3-1);
			pwm_set_chan_level(pwm_slice_num, PWM_CHAN_A, PWM_WRAP3/2); //duti ratio 50%
		}
		f>>=4;
	}
	pwm_set_clkdiv_int_frac(pwm_slice_num, f>>4, f&15);
	pwm_set_enabled(pwm_slice_num, true);
}
void sound_off(void){
	sound_on(0);
}

// 60分のn秒ウェイト（前回呼び出しからの時間）
void wait60thsec(unsigned short n){
	while(drawcount<n) asm("wfi");	// 画面描画完了までアイドルモードで待つ
	drawcount=0;
}

//演奏中の曲を1つ進める
void playmusic1step(void){
	if(music.stop) return; //演奏終了済み
	music.count--;
	if(music.count>0){
		sound_on(music.pr);
		return;
	}
	//次の音を鳴らす
	if(*music.p==254){ //曲終了
		music.stop=1;
		music.pr=0;
		sound_off();
		return;
	}
	if(*music.p==253){ //曲の最初に戻る
		music.p=music.startp;
	}
	if(*music.p==255){
		music.pr=0;
		sound_off(); //休符
	}
	else{
		music.pr=sounddata[*music.p]; //周期データ
		sound_on(music.pr);
	}
	music.p++;
	music.count=*music.p; //音符長さ
	music.p++;
}

// BGMスタート
void startmusic(const unsigned char *m){
	music.p=m;
	music.startp=m;
	music.count=1;
	music.stop=0;
}

// BGM停止
void stopmusic(void){
	music.stop=1;
	music.pr=0;
	sound_off();
}

//効果音とBGMを出力（効果音優先）
//60分の1秒ごとに呼び出し
void sound(void){
	unsigned short pr;//タイマーカウンター値

	playmusic1step();//BGMの演奏を1つ進める
	pr=2;

	//砲弾飛来音
	if(sound2count){
		if(--sound2count==0){
			if(music.stop) pr=0;
		}
		else if(!music_on){
			pr=sound2pr;
			if(sound2count>128) sound2pr-=75;//砲弾上昇時
			else sound2pr+=75;//砲弾下降時
		}
		else if((sound2count&3)==0){
			//BGMありの場合、4回に1回だけ鳴らす
			pr=sound2pr;
			if(sound2count>128) sound2pr-=300;//砲弾上昇時
			else sound2pr+=300;//砲弾下降時
		}
	}
	//その他の効果音
	if(sounddatap!=NULL){
		if(*sounddatap==0){
			sounddatap=NULL;
			if(music.stop) pr=0;
		}
		else{
			pr=*sounddatap++;
			if(pr==1) pr=0;//休符
		}
	}

	if(pr!=2) sound_on(pr);//音程変更。ただしprが2の場合BGM優先
}

//ボタン状態読み取り
//keystatus :現在押されているボタンに対応するビットを1にする
//keystatus2:前回押されていなくて、今回押されたボタンに対応するビットを1にする
void keycheck(void){
	oldkey=keystatus;
	keystatus=~gpio_get_all() & KEYSMASK;
	if(usbkb_keystatus[VK_RETURN] || usbkb_keystatus[VK_SEPARATOR]) keystatus|=KEYSTART;
	if(usbkb_keystatus[VK_SPACE]) keystatus|=KEYFIRE;
	if(usbkb_keystatus[VK_UP]) keystatus|=KEYUP;
	if(usbkb_keystatus[VK_DOWN]) keystatus|=KEYDOWN;
	if(usbkb_keystatus[VK_LEFT]) keystatus|=KEYLEFT;
	if(usbkb_keystatus[VK_RIGHT]) keystatus|=KEYRIGHT;
	keystatus2=keystatus & ~oldkey; //ボタンから手を離したかチェック
}

//停止ボタンチェック
void pausecheck(void){
	if(keystatus2&KEYSTART){
		sound_off();//サウンド停止
		do{
			wait60thsec(1);
			keycheck();
		}while((keystatus2&KEYSTART)==0);
	}
}

//逆正接関数
//戻り値 角度0-255（マップ右方向が0、時計回り）
unsigned char atan3(int x,int y){
	unsigned char a;
	if(x==0 && y==0) return 0;
	if(y>=0){
		if(x>=0){
			if(x>=y) return atandata[(int)(y*256/x)];
			else return 64-atandata[(int)(x*256/y)];
		}
		else{
			x=-x;
			if(y>=x) return 64+atandata[(int)(x*256/y)];
			else return 128-atandata[(int)(y*256/x)];
		}
	}
	else{
		y=-y;
		if(x<0){
			x=-x;
			if(x>=y) return 128+atandata[(int)(y*256/x)];
			else return 192-atandata[(int)(x*256/y)];
		}
		else{
			if(y>=x) return 192+atandata[(int)(x*256/y)];
			else return 256-atandata[(int)(y*256/x)];
		}
	}
}

//8bit乱数の種設定
void srand8(unsigned char s){
	random8=s;
}

//8bit乱数生成
unsigned char rand8(void){
	random8=random8*5+1;
	return random8;
}

// VRAM上の座標x,yから横m*縦nドット分をbmpbuf[]に取り込む
// 画面上下、左右は環状につながっている
void getbmpbuf(unsigned char x,unsigned char y,unsigned char m,unsigned char n){

	unsigned char i,j,x1;
	if(bmpbufp+m*n+4>=bmpbuf+sizeof(bmpbuf)) return;
	for(i=0;i<n;i++){
		x1=x;
		for(j=0;j<m;j++){
			*bmpbufp++=*(VRAM+((unsigned int)y<<8)+x1);
			x1++;
		}
		y++;
	}
	*bmpbufp++=x;
	*bmpbufp++=y-n;
	*bmpbufp++=m;
	*bmpbufp++=n;
}

// 横m*縦nの画像配列bmp[]を座標(x,y)に表示
// 書き込み前に元の画像をbmpbufに取り込む
void getandputbmpmn(unsigned char x,unsigned char y,unsigned char m,unsigned char n,const unsigned char bmp[]){
	getbmpbuf(x,y,m,n);
	putbmpmn3(x,y,m,n,bmp);
}

//自機の角度分座標を回転したときのx座標
int rotatex(int x,int y){
	return x*Cos(ship.angle)-y*Sin(ship.angle);
}

//自機の角度分座標を回転したときのy座標
int rotatey(int x,int y){
	return x*Sin(ship.angle)+y*Cos(ship.angle);
}

// 垂直方向にMAPにしたがって背景を描画する
// x:VRAMの描画X座標
// y1,y2:VRAM上の描画Y座標（上端、下端）
// mx,my:MAP上の座標
// d:描画する列数
// 背景データを4倍に引き伸ばす
void drawvline(unsigned char x1,unsigned char y1,unsigned char y2,int mx,int my,unsigned char d){
	unsigned char *p;
	unsigned char x,y,y3;
	x=0;
	while(d!=0){
		p=mapdataram+(((mx+x)>>1) & (MAPDX/2-1));
		y=0;
		y3=y1;
		while(y3!=y2){
//			pset(x1,y3,*(p+(((my+y)/2) & (MAPDY/2-1))*(MAPDX/2)));
			*(VRAM+y3*VRAM_X+x1)=*(p+(((my+y)>>1) & (MAPDY/2-1))*(MAPDX/2));
			y++;
			y3++;
		}
//		pset(x1,y3,*(p+(((my+y)/2) & (MAPDY/2-1))*(MAPDX/2)));
		*(VRAM+y3*VRAM_X+x1)=*(p+(((my+y)>>1) & (MAPDY/2-1))*(MAPDX/2));
		x++;
		x1++;
		d--;
	}
}

// 水平方向にMAPにしたがって背景を描画する
// y:VRAMの描画Y座標
// x1,x2:VRAM上の描画X座標（左端、右端）
// mx,my:MAP上の座標
// d:描画する行数
// 背景データを4倍に引き伸ばす
void drawhline(unsigned char y1,unsigned char x1,unsigned char x2,int mx,int my,unsigned char d){
	unsigned char *p;
	unsigned char x,y,x3;
	y=0;
	while(d!=0){
		p=mapdataram+(((my+y)>>1) & (MAPDY/2-1))*(MAPDX/2);
		x=0;
		x3=x1;
		while(x3!=x2){
//			pset(x3,y1,*(p+(((mx+x)/2) & (MAPDX/2-1))));
			*(VRAM+y1*VRAM_X+x3)=*(p+(((mx+x)>>1) & (MAPDX/2-1)));
			x++;
			x3++;
		}
//		pset(x3,y1,*(p+(((mx+x)/2) & (MAPDX/2-1))));
		*(VRAM+y1*VRAM_X+x3)=*(p+(((mx+x)>>1) & (MAPDX/2-1)));
		y++;
		y1++;
		d--;
	}
}

//背景画像のVRAMへの初期描画、vscan関連変数設定
void init_background(void){
	int dx,dy;

	//win_sx,win_sy,win_ex,win_ey 画面描画されている窓のマップ上の絶対座標の最小、最大
	//ADX1,ADY1 自機から画面左上までのマップ上の距離
	//ADX2,ADY2 自機から画面右下までのマップ上の距離
	//ship.angle 自機の進行方向角度（マップ真上向きが0、時計回り）
	if(ship.angle<64){
		win_sx=(ship.x+rotatex(-ADX1,ADY2))>>8;
		win_sy=(ship.y+rotatey(-ADX1,-ADY1))>>8;
		win_ex=(ship.x+rotatex(ADX2,-ADY1))>>8;
		win_ey=(ship.y+rotatey(ADX2,ADY2))>>8;
		vscanstartx=(ADY1+ADY2)*Sin(ship.angle);
		vscanstarty=0;
	}
	else if(ship.angle<128){
		win_sx=(ship.x+rotatex(ADX2,ADY2))>>8;
		win_sy=(ship.y+rotatey(-ADX1,ADY2))>>8;
		win_ex=(ship.x+rotatex(-ADX1,-ADY1))>>8;
		win_ey=(ship.y+rotatey(ADX2,-ADY1))>>8;
		vscanstartx=win_ex-win_sx;
		vscanstarty=-(ADY1+ADY2)*Cos(ship.angle);
	}
	else if(ship.angle<192){
		win_sx=(ship.x+rotatex(ADX2,-ADY1))>>8;
		win_sy=(ship.y+rotatey(ADX2,ADY2))>>8;
		win_ex=(ship.x+rotatex(-ADX1,ADY2))>>8;
		win_ey=(ship.y+rotatey(-ADX1,-ADY1))>>8;
		vscanstartx=-(ADX1+ADX2)*Cos(ship.angle);
		vscanstarty=win_ey-win_sy;
	}
	else{
		win_sx=(ship.x+rotatex(-ADX1,-ADY1))>>8;
		win_sy=(ship.y+rotatey(ADX2,-ADY1))>>8;
		win_ex=(ship.x+rotatex(ADX2,ADY2))>>8;
		win_ey=(ship.y+rotatey(-ADX1,ADY2))>>8;
		vscanstartx=0;
		vscanstarty=-(ADX1+ADX2)*Sin(ship.angle);
	}

	dx=win_ex-win_sx;
	dy=win_ey-win_sy;

	win_sx&=MAPDX-1;
	win_sy&=MAPDY-1;
	win_ex&=MAPDX-1;
	win_ey&=MAPDY-1;

	drawhline(0,0,dx,win_sx,win_sy,dy+1);//VRAMに描画

	//scr_sx,scr_sy,scr_ex,scr_ey 画面描画されている窓のVRAM上の左端、上端、右端、下端の列、行
	scr_sx=0;
	scr_sy=0;
	scr_ex=dx;
	scr_ey=dy;

	//自機の向きから、vscanベクトル設定
	vscanv1_x=(Cos(ship.angle)*3)>>2;
	vscanv1_y=(Sin(ship.angle)*3)>>2;
	vscanv2_x=(-Sin(ship.angle)*3)>>2;
	vscanv2_y=(Cos(ship.angle)*3)>>2;

	//r1x,r1y 画面左上の絶対座標*256
	r1x=NORMALIZEX(ship.x+rotatex(-ADX1,-ADY1));
	r1y=NORMALIZEY(ship.y+rotatey(-ADX1,-ADY1));
}

//スコア更新
//n:倒した敵の番号、255の場合は地上敵
void addscore(unsigned char n){
	if(n!=255) score+=scoretable[n];
	else score+=GNDENEMYSCORE;
	if(score>=1000000) score=999999;
	if(score>highscore) highscore=score;
}

//地上敵発生
void addgndenemy(_Enemy *ep,int x,int y){
	if(ep>=gndenemybuf+MAX_GNDENEMY) return;
	ep->on=1; //有効
	ep->no=0; //敵番号
	ep->count=rand8(); //カウンター
	ep->x=NORMALIZEX(x); //X座標*256
	ep->y=NORMALIZEY(y); //Y座標*256
	ep->vx=0; //X方向移動速度*256
	ep->vy=0; //Y方向移動速度*256
	ep->bmp=Bmp_gndenemy[0]; //画像へのポインタ
}

//空中敵発生
void addenemy(_Enemy *ep,unsigned char no,unsigned char count,int x,int y,short vx,short vy,const unsigned char *bmp){
	if(ep>=enemybuf+MAX_ENEMY) return;
	ep->no=no; //敵番号
	ep->count=count; //カウンター
	ep->x=NORMALIZEX(x); //X座標*256
	ep->y=NORMALIZEY(y); //Y座標*256
	ep->vx=vx; //X方向移動速度*256
	ep->vy=vy; //Y方向移動速度*256
	ep->bmp=bmp; //画像へのポインタ
	ep->on=1; //有効
	enemyleft++; //敵残数
}

//ミサイル発射
void addmissile(int x,int y,short vx,short vy,unsigned char c,const unsigned char *bmp){
	_Missile *p;
	for(p=missilebuf;p<missilebuf+MAX_MISSILE;p++){
		//バッファ内の空きを検索
		if(p->on) continue;
		p->on=1;
		p->x=x;
		p->y=y;
		p->vx=vx;
		p->vy=vy;
		p->bmp=bmp;
		p->count=c;
		sounddatap=sounddata1;//ミサイル発射音設定
		return;
	}
}

//砲弾発射
void addcannon(int x,int y,int vx,int vy){
	_Missile *p;
	for(p=cannonbuf;p<cannonbuf+MAX_CANNON;p++){
		//バッファ内の空きを検索
		if(p->on) continue;
		p->on=1;
		p->x=x;
		p->y=y;
		p->vx=vx;
		p->vy=vy;
		p->bmp=Bmp_cannon[0];
		p->count=255;
		sound2count=253;//砲弾飛来音カウンタ
		sound2pr=11000;//砲弾飛来音程初期値
		return;
	}
}

//自機、背景、スコア行の初期化
void gameinit4(){
	unsigned char x,n;
	ship.angle=0; //自機の向き
	ship.x=MAPDX/2*256; //自機の絶対座標*256
	ship.y=(MAPDY-42)*256;
	ship.on=1;
	clearscreen();//画面消去
	init_background();//背景初期描画
	bmpbufp=bmpbuf;

	//画面最上部にスコア、残数等描画
	printstr_fixarea(0,0,7,"STAGE");
	printnum_fixarea(40,0,7,stage);
	putbmpmn_fixarea(64,0,8,8,Bmp_enemy_mini);
	printstr_fixarea(96,0,7,"SCORE");

	//自機残数表示
	n=ships-1;
	x=256-8;
	while(n>0 && x>=24*8){
		putbmpmn_fixarea(x,0,8,8,Bmp_ship_mini);
		n--;
		x-=9;
	}
	sounddatap=NULL;//効果音なし
	sound2count=0;//砲弾飛来音なし
	sound_off();//サウンド停止
}

//ミサイル、砲弾初期化
//enemytable配列にしたがって、地上敵、空中敵の配置を初期化
void gameinit3(){
	_Enemy *p;
	_Missile *mp;
	unsigned char a,n;
	const unsigned char *tp;
	const short *pp;
	short x,y;

	srand8(gcount);//乱数の種設定
	for(mp=missilebuf;mp<missilebuf+MAX_MISSILE;mp++) mp->on=0;//ミサイル格納配列クリア
	for(mp=cannonbuf;mp<cannonbuf+MAX_CANNON;mp++) mp->on=0;//砲弾格納配列クリア
	enemyleft=0; //敵残数クリア

	if(stage<=9) tp=enemytable[stage-1];
	else tp=enemytable[9-1];//ステージ10以降はステージ9と同じ

	//空中敵の初期化
	p=enemybuf;
	while(*tp!=255){
		pp=enemyposition[*tp++];
		n=pp[0];
		x=pp[4];
		y=pp[5];
		while(n>0){
			if(p->on==1 || gamestatus!=3){ //gamestatus==3　自機1減の時、死亡敵は復活させない
				if(pp[2]>=0) a=pp[2];//負数の場合カウンタは乱数値
				else a=rand8();

				//addenemy(バッファポインタ,敵番号,カウンタ初期値,x,y,vx,vy,ビットマップ)
				addenemy(p,pp[1],a,x*256,y*256,pp[6],pp[7],Bmp_enemy[pp[1]*4+pp[3]]);
			}
			p++;
			x=(x+pp[8])&(MAPDX-1);
			y=(y+pp[9])&(MAPDY-1);
			n--;
		}
	}
	for(;p<enemybuf+MAX_ENEMY;p++) p->on=0;//バッファの終わりまでクリア

	if(gamestatus!=3){ //gamestatus==3　自機1減の時、地上敵は初期化しない
		//地上敵の初期化
		p=gndenemybuf;
		tp++;
		while(*tp!=255){
			pp=gndenemyposition[*tp++];
			n=pp[0];
			x=pp[1];
			y=pp[2];
			while(n>0){
				//addgndenemy(バッファポインタ,x,y)
				addgndenemy(p,x*256,y*256);
				p++;
				x=(x+pp[3])&(MAPDX-1);
				y=(y+pp[4])&(MAPDY-1);
				n--;
			}
		}
		for(;p<gndenemybuf+MAX_GNDENEMY;p++) p->on=0;//バッファの終わりまでクリア
	}
}

//ゲーム開始時の初期化
void gameinit2(){
	if(keystatus==(KEYFIRE|KEYLEFT|KEYSTART) && stage>1 && continuecount<CONTINUEMAX){
		continuecount++;
	}
	else{
		stage=1; //ステージ数
		continuecount=0;
	}
	ships=SHIPS; //残機数
	score=0; //得点
	if(music_on) startmusic(musicdata1);//ゲーム開始時の音楽
}

typedef struct tagBITMAPFILEHEADER {
  WORD  bfType;
  DWORD bfSize;
  WORD  bfReserved1;
  WORD  bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  DWORD biWidth;
  DWORD biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  DWORD biXPelsPerMeter;
  DWORD biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER;

// 背景画像ビットマップファイル読み込み
// 成功した場合0、失敗した場合1を返す
int read_bmp(unsigned char *f_name){
	FIL fpo;
	BITMAPFILEHEADER bmpfheader;
	BITMAPINFOHEADER bmpiheader;
	UINT br;
	uint8_t cl[4];
	int c,x,y;
	// Open BMP file
	f_mount(&FatFs, "", 0);
	if(f_open(&fpo,f_name,FA_READ)) return 1;
	int er=1;
	while(er){
		//BMPファイルヘッダー、ファイルインフォ読み込み
		if(f_read(&fpo,&bmpfheader.bfType,2,&br)) break;
		if(bmpfheader.bfType!=('B'+'M'*256)) break;
		//bfSize以降2バイトずれているので分けて読み込む
		if(f_read(&fpo,&bmpfheader.bfSize,12,&br)) break;
		if(f_read(&fpo,&bmpiheader,sizeof bmpiheader,&br)) break;
		if(bmpiheader.biWidth!=MAPBMPDX || bmpiheader.biHeight!=MAPBMPDY || bmpiheader.biBitCount!=8) break;
		//カラーパレット読み込み、設定（パレット番号128以降に設定）
		for(c=128;c<=255;c++){
			if(f_read(&fpo,cl,4,&br)) break;
			set_palette(c,cl[0],cl[2],cl[1]);
		}
		if(c<256) break;
		if(f_lseek(&fpo,bmpfheader.bfOffBits)) break;
		//画像データ読み込み、パレット番号は128足す
		for(y=MAPBMPDY-1;y>=0;y--){
			if(f_read(&fpo,mapdataram+y*MAPBMPDX,MAPBMPDX,&br)) break;
			uint8_t *p=mapdataram+y*MAPBMPDX;
			for(x=0;x<MAPBMPDX;x++){
				*p+=128;
				p++;
			}
		}
		if(y>=0) break;
		er=0;
	}
	// Close file
	f_close(&fpo);
	return er;
}

//ゲーム全体初期化
void gameinit(){
	//カラーパレット設定
	int i,j;
	unsigned char d;
	const unsigned char *p;
	// 背景画像読み込み
	// 読み込みできなかった場合は標準画像を使用
	if(read_bmp("VELUDDA.BMP")){
		p=paldata2;
		for(i=128;i<256;i++){
			set_palette(i,*p,*(p+2),*(p+1));
			p+=3;
		}
		p=mapdata;
		for(i=0;i<MAPBMPDY;i+=2){
			for(j=0;j<MAPBMPDX;j+=2){
				mapdataram[i*MAPBMPDX+j]=*p;
				mapdataram[i*MAPBMPDX+j+1]=*p;
				mapdataram[(i+1)*MAPBMPDX+j]=*p;
				mapdataram[(i+1)*MAPBMPDX+j+1]=*p;
				p++;
			}
		}
	}
	gcount=0;//全体カウンター
	score=0;
	highscore=0;
	music_on=1;//BGMあり
	stopmusic();//BGM停止中
	continuecount=0;
}

//タイトル画面描画
void title(){
	const unsigned char *p;
	unsigned char x,y,c,n;
	int i;
	clearscreen();//画面消去
	initvscanv();//vscan関連変数初期化（拡大縮小なし、回転なし）

	//ロゴ描画
	p=Bmp_logo;
	y=0;
	x=0;
	while(y<YSIZE_LOGO){
		c=*p++;//パレット番号
		n=*p++;//ランレングス
		while(n>0){
			pset(x+23,y+10,c);
			n--;
			x++;
			if(x>=XSIZE_LOGO){
				x=0;
				y++;
			}
		}
	}
	printstr2(128-3*8,90-24,7,"SPEED UP");
	printstr2(128-11*8,90+8,7,"TURN LEFT    TURN RIGHT");
	putfont(128-4,90-2*8,6,0x1e);//「↑」
	putfont(128-20,90-4,6,0x1d);//「←」
	putfont(128+12,90-4,6,0x1c);//「→」
	putbmpmn3(128-XSIZE_SHIP/2,90-YSIZE_SHIP/2,XSIZE_SHIP,YSIZE_SHIP,Bmp_ship);
	printstr2(128-11*8,90+24,7,"MISSILE:      BUTTON");
	printstr2(128-11*8+9*8,90+24,6,"FIRE");
	printstr2(0,150,7,"HI-SCORE");
	printnum2(10*8,150,7,highscore);
	printstr2(144,150,7,"SCORE");
	printnum2(144+7*8,150,7,score);
	printstr2(128-4*8,170,7,"BY KENKEN");
	printstr2(128-2*8,180,7,"WITH");
	printstr2(128-16*8,190,7,"NTSC VIDEO ROTARY OUTPUT SYSTEM");
	printstr2(128-11*8,200,7,"FOR RASPBERRY PI PICO");
	printstr2(256-9*8,130,7,"MUSIC");
	putfont2(256-4*8,124,6,0x1e);//「↑」
	putfont2(256-4*8,136,6,0x1f);//「↓」
	while(1){
		//拡大画面から回転しながら通常サイズに戻す、回転中心はロゴ部分から徐々に下げる
		for(x=254;x>0;x-=2){
			wait60thsec(1);
			gcount++;//全体カウンタ
			vscanv1_x=50*Cos(x)/(x+50);
			vscanv1_y=50*Sin(x)/(x+50);
			vscanv2_x=-vscanv1_y;
			vscanv2_y=vscanv1_x;
			vscanstartx=50*((int)(-128)*Cos(x)+108*Sin(x))/(x+50)+128*256;
			vscanstarty=50*((int)(-128)*Sin(x)-108*Cos(x))/(x+50)+108*256-Sin(x/4)*88;
			keycheck();
			if(keystatus & KEYSTART) return;//ゲームスタート
			if(keystatus2 & KEYUP) music_on=1;
			else if(keystatus2 & KEYDOWN) music_on=0;
			if(music_on){
				printstr2(256-3*8,124, 4,"ON");
				printstr2(256-3*8,136,12,"OFF");
			}
			else{
				printstr2(256-3*8,124,12,"ON");
				printstr2(256-3*8,136, 4,"OFF");
			}
		}
		initvscanv();
		for(i=0;i<15*60;i++){ //15秒間
			wait60thsec(1);
			gcount++;//全体カウンタ
			keycheck();
			if(keystatus & KEYSTART) return;//ゲームスタート
			if(keystatus2 & KEYUP) music_on=1;
			else if(keystatus2 & KEYDOWN) music_on=0;
			if(music_on){
				printstr2(256-3*8,124, 4,"ON");
				printstr2(256-3*8,136,12,"OFF");
			}
			else{
				printstr2(256-3*8,124,12,"ON");
				printstr2(256-3*8,136, 4,"OFF");
			}
		}
	}
}

//画面スクロール＆背景描画
//スクロール前後での差分のみ描画する
void scroll_drawground(){
	int oldsx,oldsy,oldex,oldey;
	int oldr1x,oldr1y;
	signed char d;

	oldr1x=r1x;
	oldr1y=r1y;
	r1x=NORMALIZEX(ship.x+rotatex(-ADX1,-ADY1));
	r1y=NORMALIZEY(ship.y+rotatey(-ADX1,-ADY1));

	//自機が移動、回転した分、vscanstartx,vscanstartyを移動させる
	vscanstartx+=(unsigned short)(r1x-oldr1x);
	vscanstarty+=(unsigned short)(r1y-oldr1y);

	//自機の回転に合わせ、vscanベクトルを変更
	vscanv1_x=(Cos(ship.angle)*3)>>2;
	vscanv1_y=(Sin(ship.angle)*3)>>2;
	vscanv2_x=(-Sin(ship.angle)*3)>>2;
	vscanv2_y=(Cos(ship.angle)*3)>>2;

	//描画窓移動し、未描画部分を新たに描画する
	oldsx=win_sx;
	oldsy=win_sy;
	oldex=win_ex;
	oldey=win_ey;
	if(ship.angle<64){
		win_sx=(ship.x+rotatex(-ADX1,ADY2))>>8;
		win_sy=(ship.y+rotatey(-ADX1,-ADY1))>>8;
		win_ex=(ship.x+rotatex(ADX2,-ADY1))>>8;
		win_ey=(ship.y+rotatey(ADX2,ADY2))>>8;
	}
	else if(ship.angle<128){
		win_sx=(ship.x+rotatex(ADX2,ADY2))>>8;
		win_sy=(ship.y+rotatey(-ADX1,ADY2))>>8;
		win_ex=(ship.x+rotatex(-ADX1,-ADY1))>>8;
		win_ey=(ship.y+rotatey(ADX2,-ADY1))>>8;
	}
	else if(ship.angle<192){
		win_sx=(ship.x+rotatex(ADX2,-ADY1))>>8;
		win_sy=(ship.y+rotatey(ADX2,ADY2))>>8;
		win_ex=(ship.x+rotatex(-ADX1,ADY2))>>8;
		win_ey=(ship.y+rotatey(-ADX1,-ADY1))>>8;
	}
	else{
		win_sx=(ship.x+rotatex(-ADX1,-ADY1))>>8;
		win_sy=(ship.y+rotatey(ADX2,-ADY1))>>8;
		win_ex=(ship.x+rotatex(ADX2,ADY2))>>8;
		win_ey=(ship.y+rotatey(-ADX1,ADY2))>>8;
	}
	//マップ右端、下端からはみ出した部分を正規化
	win_sx&=MAPDX-1;
	win_sy&=MAPDY-1;
	win_ex&=MAPDX-1;
	win_ey&=MAPDY-1;

	//画面の上下左右の空白部分を新たに描画する
	d=win_sx-oldsx;
	scr_sx+=d;
	if(d<0){
		//左側空き場所を描画
		drawvline(scr_sx,scr_sy,scr_ey,win_sx,oldsy,(-d));
	}
	d=win_ex-oldex;
	scr_ex+=d;
	if(d>0){
		//右側空き場所を描画
		drawvline(scr_ex-d+1,scr_sy,scr_ey,win_ex-d+1,oldsy,d);
	}
	d=win_sy-oldsy;
	scr_sy+=d;
	if(d<0){
		//上側空き場所を描画
		drawhline(scr_sy,scr_sx,scr_ex,win_sx,win_sy,(-d));
	}
	d=win_ey-oldey;
	scr_ey+=d;
	if(d>0){
		//下側空き場所を描画
		drawhline(scr_ey-d+1,scr_sx,scr_ex,win_sx,win_ey-d+1,d);
	}
}

//自機表示
void drawship(){
	if(ship.on==1){
		//通常
		getandputbmpmn(((ship.x>>8)-win_sx+scr_sx-XSIZE_SHIP/2)&0xff,((ship.y>>8)-win_sy+scr_sy-YSIZE_SHIP/2)&0xff,
			XSIZE_SHIP,YSIZE_SHIP,Bmp_ship);
	}
	else if(ship.on==2){
		//爆発画像
		getandputbmpmn(((ship.x>>8)-win_sx+scr_sx-XSIZE_EXPLODE2/2)&0xff,((ship.y>>8)-win_sy+scr_sy-YSIZE_EXPLODE2/2)&0xff,
			XSIZE_EXPLODE2,YSIZE_EXPLODE2,Bmp_explode2[ship.count>>2]);
		ship.count++;
		if(ship.count==32) ship.on=0;//爆発終了
	}
}

//地上敵表示
void drawgndenemy(){
	_Enemy *p;
	const unsigned char *bmp;
	unsigned short x,y;
	for(p=gndenemybuf;p<gndenemybuf+MAX_GNDENEMY;p++){
		if(!p->on) continue;
		//画面描画窓内に入っているかチェック
		x=p->x>>8;
		y=p->y>>8;
		if(win_sx<win_ex){ //描画窓の横方向がマップ内に収まっている場合
			if(x<win_sx) continue; //描画窓より左にいる
			if(x>win_ex) continue; //描画窓より右にいる
		}
		else{ //描画窓がマップ右端にまたがっている場合
			if(x>win_ex && x<win_sx) continue; //描画窓の外にいる
		}
		if(win_sy<win_ey){ //描画窓の上下方向がマップ内に収まっている場合
			if(y<win_sy) continue; //描画窓より上にいる
			if(y>win_ey) continue; //描画窓より下にいる
		}
		else{ //描画窓がマップ下端にまたがっている場合
			if(y>win_ey && y<win_sy) continue; //描画窓の外にいる
		}
		if(p->on==2){
			//爆発時
			getandputbmpmn((x-win_sx+scr_sx-XSIZE_EXPLODE/2)&0xff,(y-win_sy+scr_sy-YSIZE_EXPLODE/2)&0xff,
				XSIZE_EXPLODE,YSIZE_EXPLODE,Bmp_explode[p->count]);
			p->count++;
			if(p->count==8) p->on=3;//爆発後
			continue;
		}
		if(p->on==3) bmp=Bmp_gndenemy[4]; //爆発後
		else if(p->count<208) bmp=Bmp_gndenemy[0]; //通常
		else bmp=Bmp_gndenemy[((p->count-208)>>4)+1]; //砲弾発射前
		getandputbmpmn((x-win_sx+scr_sx-XSIZE_GNDENEMY/2)&0xff,(y-win_sy+scr_sy-YSIZE_GNDENEMY/2)&0xff,
			XSIZE_GNDENEMY,YSIZE_GNDENEMY,bmp);
	}
}

//空中敵描画
void drawenemy(){
	_Enemy *p;
	unsigned short x,y;
	for(p=enemybuf;p<enemybuf+MAX_ENEMY;p++){
		if(!p->on) continue;

		//画面描画窓内に入っているかチェック
		x=p->x>>8;
		y=p->y>>8;
		if(win_sx<win_ex){
			if(x<win_sx) continue;
			if(x>win_ex) continue;
		}
		else{
			if(x>win_ex && x<win_sx) continue;
		}
		if(win_sy<win_ey){
			if(y<win_sy) continue;
			if(y>win_ey) continue;
		}
		else{
			if(y>win_ey && y<win_sy) continue;
		}
		if(p->on==1){
			//通常時
			getandputbmpmn((x-win_sx+scr_sx-XSIZE_ENEMY/2)&0xff,(y-win_sy+scr_sy-YSIZE_ENEMY/2)&0xff,
				XSIZE_ENEMY,YSIZE_ENEMY,p->bmp);
		}
		else{
			//爆発時
			getandputbmpmn((x-win_sx+scr_sx-XSIZE_EXPLODE/2)&0xff,(y-win_sy+scr_sy-YSIZE_EXPLODE/2)&0xff,
				XSIZE_EXPLODE,YSIZE_EXPLODE,Bmp_explode[p->count]);
			p->count++;
			if(p->count==8) p->on=0;//消滅
		}
	}
}

//ミサイル描画
void drawmissile(){
	_Missile *p;
	unsigned short x,y;
	for(p=missilebuf;p<missilebuf+MAX_MISSILE;p++){
		if(!p->on) continue;

		//画面描画窓内に入っているかチェック
		x=p->x>>8;
		y=p->y>>8;
		if(win_sx<win_ex){
			if(x<win_sx) continue;
			if(x>win_ex) continue;
		}
		else{
			if(x>win_ex && x<win_sx) continue;
		}
		if(win_sy<win_ey){
			if(y<win_sy) continue;
			if(y>win_ey) continue;
		}
		else{
			if(y>win_ey && y<win_sy) continue;
		}
		getandputbmpmn((x-win_sx+scr_sx-XSIZE_MISSILE/2)&0xff,(y-win_sy+scr_sy-YSIZE_MISSILE/2)&0xff,
			XSIZE_MISSILE,YSIZE_MISSILE,p->bmp);
	}
}

//砲弾描画
void drawcannon(){
	_Missile *p;
	unsigned short x,y;
	for(p=cannonbuf;p<cannonbuf+MAX_CANNON;p++){
		if(!p->on) continue;

		//画面描画窓内に入っているかチェック
		x=p->x>>8;
		y=p->y>>8;
		if(win_sx<win_ex){
			if(x<win_sx) continue;
			if(x>win_ex) continue;
		}
		else{
			if(x>win_ex && x<win_sx) continue;
		}
		if(win_sy<win_ey){
			if(y<win_sy) continue;
			if(y>win_ey) continue;
		}
		else{
			if(y>win_ey && y<win_sy) continue;
		}
		getandputbmpmn((x-win_sx+scr_sx-XSIZE_CANNON/2)&0xff,(y-win_sy+scr_sy-YSIZE_CANNON/2)&0xff,
			XSIZE_CANNON,YSIZE_CANNON,p->bmp);
	}
}

//敵残数、スコアの表示
void drawscore(){
	putfont_fixarea(10*8,0,0,' ');
	printnum_fixarea(9*8,0,7,enemyleft);
	printnum_fixarea(18*8,0,7,score);
}

//bmpbufに格納された画面内容を逆順に再描画して背景画像に戻す
void erasechars(){
	unsigned char m,n;
	while(bmpbufp>bmpbuf){
		m=*(bmpbufp-2);
		n=*(bmpbufp-1);
		putbmpmn2(*(bmpbufp-4),*(bmpbufp-3),m,n,bmpbufp-4-m*n);
		bmpbufp-=4+m*n;
	}
}

//自機の移動
void moveship(){
	if(ship.on!=1) return;
	if(keystatus & KEYLEFT) ship.angle--;
	if(keystatus & KEYRIGHT) ship.angle++;
	if(keystatus & KEYUP){
		//上ボタンで1.5倍速
		ship.x=NORMALIZEX(ship.x+Sin(ship.angle)+Sin(ship.angle)/2);
		ship.y=NORMALIZEY(ship.y-Cos(ship.angle)-Cos(ship.angle)/2);
	}
	else{
		ship.x=NORMALIZEX(ship.x+Sin(ship.angle));
		ship.y=NORMALIZEY(ship.y-Cos(ship.angle));
	}
}

//地上敵の砲弾発射処理
void movegndenemy(){
	_Enemy *p;
	int dx,dy,dx1,dy1;
	if(ship.on!=1) return;
	for(p=gndenemybuf;p<gndenemybuf+MAX_GNDENEMY;p++){
		//p->on 0:無効、1:通常、2:爆発中、3:爆発後
		if(p->on!=1) continue;
		//カウンタが0で自機に近いと狙って砲弾発射
		if(++p->count) continue;
		dx=ship.x-p->x;
		if(dx<0) dx1=-dx; else dx1=dx;
		if(dx1>(MAPDX-CANNONFIRE)*256){
			if(dx<0) dx+=MAPDX*256;
			else dx-=MAPDX*256;
		}
		else if(dx1>=CANNONFIRE*256) continue;
		dy=ship.y-p->y;
		if(dy<0) dy1=-dy; else dy1=dy;
		if(dy1>(MAPDY-CANNONFIRE)*256){
			if(dy<0) dy+=MAPDY*256;
			else dy-=MAPDY*256;
		}
		else if(dy1>=CANNONFIRE*256) continue;
		addcannon(p->x,p->y,dx/128+Sin(ship.angle),dy/128-Cos(ship.angle)); //砲弾発射
	}
}

//敵が自機の近傍にいるかチェック
//戻り値　近傍の場合：1、それ以外：0
unsigned char enemynearcheck(_Enemy *p){
#define ENEMYNEAR 80 //近傍距離

	int dx,dy;

	if(ship.on!=1) return 0;
	dx=ship.x-p->x;
	if(dx<0) dx=-dx;
	dx>>=8;
	if(dx>=(MAPDX-ENEMYNEAR)) dx-=MAPDX;
	else if(dx>ENEMYNEAR) return 0;
	dy=ship.y-p->y;
	if(dy<0) dy=-dy;
	dy>>=8;
	if(dy>=(MAPDY-ENEMYNEAR)) dy-=MAPDY;
	else if(dy>ENEMYNEAR) return 0;
	if((dx*dx+dy*dy)>ENEMYNEAR*ENEMYNEAR) return 0;
	return 1;
}

//自機を避けるように敵の進行方向変更
void enemyavoid(_Enemy *p){
	int dx,dy;
	unsigned char a1,a2,a3;

	if(ship.on!=1) return;
	dx=(ship.x-p->x)>>8;
	if(dx<-MAPDX/2) dx+=MAPDX;
	else if(dx>MAPDX/2) dx-=MAPDX;
	dy=(ship.y-p->y)>>8;
	if(dy<-MAPDY/2) dy+=MAPDY;
	else if(dy>MAPDY/2) dy-=MAPDY;

	a1=atan3(dx,dy); //敵と自機の角度
	a2=atan3(p->vx,p->vy); //敵進行方向角度
	a3=a1-a2;
	//自機を避ける方向に旋回させる
	if(a3<96){
		a2-=4;
		p->vx=Cos(a2);
		p->vy=Sin(a2);
	}
	if(a3>256-96){
		a2+=4;
		p->vx=Cos(a2);
		p->vy=Sin(a2);
	}
}

// 空中敵を時計回りに90度回転
void enemyturnright(_Enemy *p){
	short t1;
	t1=p->vx;
	p->vx=-(p->vy);
	p->vy=t1;
	p->bmp+=SQSIZE_ENEMY;
	if(p->bmp==Bmp_enemy[p->no*4+4]) p->bmp=Bmp_enemy[p->no*4];
}

// 空中敵を反時計回りに90度回転
void enemyturnleft(_Enemy *p){
	short t1;
	t1=p->vx;
	p->vx=p->vy;
	p->vy=-t1;
	p->bmp-=SQSIZE_ENEMY;
	if(p->bmp<Bmp_enemy[p->no*4]) p->bmp=Bmp_enemy[p->no*4+3];
}

//空中敵の移動
void moveenemy(){
	_Enemy *p;
	for(p=enemybuf;p<enemybuf+MAX_ENEMY;p++){
		//p->on 0:無効、1:通常、2:爆発中
		if(p->on!=1) continue;
		//通常の場合
		p->x=NORMALIZEX(p->x + p->vx);
		p->y=NORMALIZEY(p->y + p->vy);
		p->count++;
		switch(p->no){
			case 0:
				//何もしない（等速直線運動）
				break;
			case 1:
				//自機に近づくと回避行動
				if(enemynearcheck(p)) enemyavoid(p);
				if((p->count&7)==0){
					p->bmp+=SQSIZE_ENEMY;
					if(p->bmp==Bmp_enemy[1*4+4]) p->bmp=Bmp_enemy[1*4];
				}
				break;
			case 2:
				//ジグザグ運動
				if(p->count&0x1f) break;
				if(p->count&0x20) enemyturnright(p);//90度正方向回転
				else enemyturnleft(p);//90度負方向回転
				break;
			case 3:
				if(p->count<208) p->bmp=Bmp_enemy[3*4];
				else p->bmp=Bmp_enemy[3*4+1+((p->count-208)>>4)];
				if(p->count==0 && enemynearcheck(p)){
					//カウンタが0で自機が近くにいる場合4方向にミサイル発射
					addmissile(NORMALIZEX(p->x-8*256),p->y,-256*2,0,60,Bmp_missile2);
					addmissile(NORMALIZEX(p->x+8*256),p->y, 256*2,0,60,Bmp_missile2);
					addmissile(p->x,NORMALIZEY(p->y-8*256),0,-256*2,60,Bmp_missile2);
					addmissile(p->x,NORMALIZEY(p->y+8*256),0, 256*2,60,Bmp_missile2);
				}
				break;
			case 4:
				//正弦波運動
				p->vy=Cos(p->count)/2-256;
				p->bmp=Bmp_enemy[4*4+(p->count >>6)];
				break;
			case 5:
				//回転運動
				p->vx=Sin(p->count);
				p->vy=Cos(p->count);
				if((p->count&7)==0){
					p->bmp+=SQSIZE_ENEMY;
					if(p->bmp==Bmp_enemy[5*4+4]) p->bmp=Bmp_enemy[5*4];
				}
				break;
			default:
				break;
		}
	}
}

//ミサイル移動
void movemissile(){
	_Missile *p;
	for(p=missilebuf;p<missilebuf+MAX_MISSILE;p++){
		if(!p->on) continue;
		if(--p->count==0){
			//ミサイル消滅
			p->on=0;
			continue;
		}
		p->x=NORMALIZEX(p->x + p->vx);
		p->y=NORMALIZEY(p->y + p->vy);
	}
}

//砲弾移動
void movecannon(){
	_Missile *p;
	int c;
	for(p=cannonbuf;p<cannonbuf+MAX_CANNON;p++){
		if(!p->on) continue;
		if(--p->count==0){
			//砲弾消滅
			p->on=0;
			continue;
		}
		p->x=NORMALIZEX(p->x + p->vx);
		p->y=NORMALIZEY(p->y + p->vy);
		c=p->count >>4;
		if(c>7) c=15-c;
		p->bmp=Bmp_cannon[c];
	}
}

//FIREボタンで自機のミサイル発射
void fire(){
	if(ship.on!=1) return;
	if(keystatus2 & KEYFIRE){
		addmissile(NORMALIZEX(ship.x+Sin(ship.angle)*12),NORMALIZEY(ship.y-Cos(ship.angle)*12),
			Sin(ship.angle)*5,-Cos(ship.angle)*5,32,Bmp_missile1);
	}
}

//空中敵死亡処理
void enemydeath(_Enemy *ep){
	ep->on=2;//敵爆発
	ep->count=0; //敵爆発画像用カウンタ
	enemyleft--; //敵残数減
	addscore(ep->no);
	sounddatap=sounddata2;//敵爆発音設定
}

//地上敵死亡処理
void gndenemydeath(_Enemy *ep){
	ep->on=2;//敵爆発
	ep->count=0; //敵爆発画像用カウンタ
	addscore(255);
	sounddatap=sounddata2;//敵爆発音設定
}

//自機死亡処理
void shipdeath(){
	ship.on=2;//爆発中
	ship.count=0; //自機爆発画像用カウンタ
	sounddatap=sounddata3;//自機爆発音設定
	sound2count=0;//砲弾飛来音停止
}

//各種衝突チェック
void collisioncheck(){
	_Enemy *ep;
	_Missile *mp;
	int dx,dy;

	//ミサイルと空中敵の衝突チェック
	for(mp=missilebuf;mp<missilebuf+MAX_MISSILE;mp++){
		if(!mp->on) continue;
		for(ep=enemybuf;ep<enemybuf+MAX_ENEMY;ep++){
			if(ep->on!=1) continue;
			//X座標チェック
			dx=mp->x - ep->x;
			if(dx<0) dx=-dx;
			if(dx>XSIZE_ENEMY/2*256 && dx<(MAPDX-XSIZE_ENEMY/2)*256) continue;
			//Y座標チェック
			dy=mp->y - ep->y;
			if(dy<0) dy=-dy;
			if(dy>YSIZE_ENEMY/2*256 && dy<(MAPDY-YSIZE_ENEMY/2)*256) continue;

			// ミサイルと敵が衝突した時の処理
			enemydeath(ep);//敵死亡処理
			mp->on=0; //ミサイル消滅
		}
	}
	//砲弾と空中敵の衝突チェック
	for(mp=cannonbuf;mp<cannonbuf+MAX_CANNON;mp++){
		if(!mp->on) continue;
		if(mp->count<112 || mp->count>=144) continue;//砲弾の高度チェック
		for(ep=enemybuf;ep<enemybuf+MAX_ENEMY;ep++){
			if(ep->on!=1) continue;
			//X座標チェック
			dx=mp->x - ep->x;
			if(dx<0) dx=-dx;
			if(dx>(XSIZE_CANNON/2+XSIZE_ENEMY/2)*256 && dx<(MAPDX-(XSIZE_CANNON/2+XSIZE_ENEMY/2))*256) continue;
			//Y座標チェック
			dy=mp->y - ep->y;
			if(dy<0) dy=-dy;
			if(dy>(YSIZE_CANNON/2+YSIZE_ENEMY/2)*256 && dy<(MAPDY-(YSIZE_CANNON/2+YSIZE_ENEMY/2))*256) continue;

			// 砲弾と空中敵が衝突した時の処理
			enemydeath(ep);//敵死亡処理
			mp->on=0; //砲弾消滅
		}
	}
	//砲弾と地上敵の衝突チェック
	for(mp=cannonbuf;mp<cannonbuf+MAX_CANNON;mp++){
		if(!mp->on) continue;
		if(mp->count>1) continue;//砲弾の高度チェック
		for(ep=gndenemybuf;ep<gndenemybuf+MAX_GNDENEMY;ep++){
			if(ep->on!=1) continue;
			//X座標チェック
			dx=mp->x - ep->x;
			if(dx<0) dx=-dx;
			if(dx>(XSIZE_CANNON/2+XSIZE_GNDENEMY/2)*256 && dx<(MAPDX-(XSIZE_CANNON/2+XSIZE_GNDENEMY/2))*256) continue;
			//Y座標チェック
			dy=mp->y - ep->y;
			if(dy<0) dy=-dy;
			if(dy>(YSIZE_CANNON/2+YSIZE_GNDENEMY/2)*256 && dy<(MAPDY-(YSIZE_CANNON/2+YSIZE_GNDENEMY/2))*256) continue;

			// 砲弾と地上敵が衝突した時の処理
			gndenemydeath(ep);//地上敵爆発処理
			mp->on=0; //砲弾消滅
		}
	}

//ここから自機衝突チェック
	if(ship.on!=1) return; //自機死亡中
	//自機と空中敵の衝突チェック
	for(ep=enemybuf;ep<enemybuf+MAX_ENEMY;ep++){
		if(ep->on!=1) continue;
		//X座標チェック
		dx=ship.x-ep->x;
		if(dx<0) dx=-dx;
		if(dx>(XSIZE_SHIP/2+XSIZE_ENEMY/2-2)*256 && dx<(MAPDX-(XSIZE_SHIP/2+XSIZE_ENEMY/2-2))*256) continue;
		//Y座標チェック
		dy=ship.y-ep->y;
		if(dy<0) dy=-dy;
		if(dy>(YSIZE_SHIP/2+YSIZE_ENEMY/2-2)*256 && dy<(MAPDY-(YSIZE_SHIP/2+YSIZE_ENEMY/2-2))*256) continue;

		// 自機と敵が衝突した時の処理
		enemydeath(ep);//敵死亡処理
		shipdeath();//自機死亡処理
	}
	//自機と砲弾の衝突チェック
	for(mp=cannonbuf;mp<cannonbuf+MAX_CANNON;mp++){
		if(mp->on!=1) continue;
		if(mp->count<112 || mp->count>=144) continue;//砲弾の高度チェック
		//X座標チェック
		dx=ship.x-mp->x;
		if(dx<0) dx=-dx;
		if(dx>(XSIZE_SHIP/2+XSIZE_CANNON/2-2)*256 && dx<(MAPDX-(XSIZE_SHIP/2+XSIZE_CANNON/2-2))*256) continue;
		//Y座標チェック
		dy=ship.y-mp->y;
		if(dy<0) dy=-dy;
		if(dy>(YSIZE_SHIP/2+YSIZE_CANNON/2-2)*256 && dy<(MAPDY-(YSIZE_SHIP/2+YSIZE_CANNON/2-2))*256) continue;

		// 自機と砲弾が衝突した時の処理
		mp->on=0;//砲弾消滅
		shipdeath();//自機死亡処理
	}

	//自機とミサイルの衝突チェック
	for(mp=missilebuf;mp<missilebuf+MAX_MISSILE;mp++){
		if(mp->on!=1) continue;
		//X座標チェック
		dx=ship.x-mp->x;
		if(dx<0) dx=-dx;
		if(dx>(XSIZE_SHIP/2-2)*256 && dx<(MAPDX-(XSIZE_SHIP/2-2))*256) continue;
		//Y座標チェック
		dy=ship.y-mp->y;
		if(dy<0) dy=-dy;
		if(dy>(YSIZE_SHIP/2-2)*256 && dy<(MAPDY-(YSIZE_SHIP/2-2))*256) continue;

		// 自機とミサイルが衝突した時の処理
		mp->on=0;//ミサイル消滅
		shipdeath();//自機死亡処理
	}
}

//ゲーム終了時処理
void gameover(){
	sound_off();//サウンド停止
}

//ゲームのステータス更新
void changegamestatus(){
	unsigned char x,y,e;
	switch(gamestatus){
		case 0://ゲームスタート
		case 1://ステージ数表示中
			getbmpbuf(64,80,72,8);
			printstr2(64,80,7,"STAGE");
			printnum2(64+8*6,80,7,stage);
			if(stage<=6){
				//空中敵名称と得点表示
				e=enemyposition[enemytable[stage-1][0]][1];
				getbmpbuf(64,90,12*8,8);
				printstr2(64,90,7,enemyname[e]);
				printnum2(64+7*8,90,7,scoretable[e]);
				printstr2(64+9*8,90,7,"PTS");
			}
			gamestatuscount--;
			if(gamestatuscount==0){
				if(music_on) startmusic(musictable[(stage-1)%(sizeof(musictable)/sizeof(musictable[0]))]);//BGM開始
				gamestatus=2;
			}
			break;
		case 2://通常ゲーム中
			if(ship.on!=1){
				stopmusic();//曲演奏停止
				gamestatus=3;//自機死亡
				gamestatuscount=180;//3秒停止
				break;
			}
			if(enemyleft==0){
				//敵全滅
				stopmusic();//曲演奏停止
				sound2count=0;//砲弾飛来音停止
				gamestatus=4; //ステージクリア
				gamestatuscount=120; //2秒間停止
			}
			break;
		case 3://自機死亡
			if(--gamestatuscount) break;
			ships--;
			if(ships==0){
				gamestatus=5; //ゲームオーバー
				gamestatuscount=300; //5秒間「GAMEOVER」表示
				break;
			}
			if(enemyleft>0){
				gameinit3();//ステージ初期化
				gameinit4();//自機初期化
				gamestatus=1; //ステージ最初から
				gamestatuscount=180; //3秒間ステージ数表示
				break;
			}
			//次ステージへ
			gamestatus=4;
			gamestatuscount=1;
		case 4://次ステージへ
			if(--gamestatuscount) break;
			if(stage<99) stage++;
			if(stage%5==0 && ships<255) ships++;//5ステージごとに自機+1
			gameinit3();//ステージ初期化
			gameinit4();//自機初期化
			gamestatus=1; //ステージ最初へ
			gamestatuscount=180; //3秒間ステージ数表示
			break;
		case 5:	//ゲームオーバー
			//画面回転位置を戻し、中央にGAMEOVER表示
			x=(vscanstartx>>8)+64;
			y=(vscanstarty>>8)+80;
			getbmpbuf(x,y,72,8);
			printstr2(x,y,(gcount&3)+5,"GAMEOVER");
			gamestatuscount--;
			if(gamestatuscount==0) gamestatus=6;
			if(ship.angle==0) break;
			// GAMEOVER表示を正常な向きに戻す
			if(ship.angle<128) ship.angle--;
			else ship.angle++;
			break;
	}
}

//メインループ
void game(){
	gamestatus=0;//0:ゲーム開始、1:ステージ数表示中、2:ゲーム中、3:プレイヤー1減、4:ステージクリア、5:ゲームオーバー表示、6:終了
	if(music_on) gamestatuscount=500;
	else gamestatuscount=180;
	gameinit2();//ゲーム開始処理
	gameinit3();//ステージクリア処理
	gameinit4();//自機初期化
	while(gamestatus<6){
		wait60thsec(1);
		sound();//サウンド処理
		erasechars();//背景以外の描画消去
		keycheck();//ボタンスキャン
		if(gamestatus==2 || gamestatus==3){
			fire();//自機ミサイル発射処理
			moveship();//自機移動処理
			movegndenemy();//地上敵移動処理（砲弾発射）
			moveenemy();//空中敵移動処理
			movemissile();//ミサイル移動処理
			movecannon();//砲弾移動処理
			collisioncheck();//各種衝突チェック
		}
		scroll_drawground();//スクロール＆背景描画
		drawgndenemy();//地上敵描画
		drawcannon();//砲弾描画
		drawenemy();//空中敵描画
		drawship();//自機描画
		drawmissile();//ミサイル描画
		drawscore();//得点描画
		changegamestatus();//ゲームのステータス更新処理
//		pausecheck();//停止ボタンチェック
		gcount++;//全体カウンタ

	}
	gameover(); //ゲーム終了処理
}

void core1_entry(void){
	while(1){
		usbkb_polling();
		sleep_ms(10);
	}
}

int main(void){
    stdio_init_all();

	// ボタン用GPIO設定
	gpio_init_mask(KEYSMASK);
	gpio_init_mask(GPIO_ALL_MASK);
	gpio_set_dir_in_masked(KEYSMASK);
	gpio_pull_up(GPIO_KEYUP);
	gpio_pull_up(GPIO_KEYLEFT);
	gpio_pull_up(GPIO_KEYRIGHT);
	gpio_pull_up(GPIO_KEYDOWN);
	gpio_pull_up(GPIO_KEYSTART);
	gpio_pull_up(GPIO_KEYFIRE);

	init_composite(VIDEOPORT); // NTSCカラー出力制御初期化

	// サウンド用PWM設定
	gpio_set_function(SOUNDPORT, GPIO_FUNC_PWM);
	pwm_slice_num = pwm_gpio_to_slice_num(SOUNDPORT);
	sound_on(0);

	lockkey=1; // 下位3ビットが<SCRLK><CAPSLK><NUMLK>
	keytype=0; // 0：日本語109キー、1：英語104キー
	usbkb_init();
	multicore_launch_core1(core1_entry);

	gameinit(); //ゲーム全体初期化
	while(1){
		title();//タイトル画面、スタートボタンで戻る
		game();//ゲームメインループ
	}
}
