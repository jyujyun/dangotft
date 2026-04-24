#ifndef DANGOTFT_H
#define DANGOTFT_H
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"

#include "hardware/spi.h"
//キーコンフィグたち
#define PINR 15
#define PIND 4
#define PINL 5
#define PINU 14

#define PINJX 26
#define PINJY 27

#define INPUT_LEFT (gpio_get(PINL) == 0)
#define INPUT_UP (gpio_get(PINU) == 0)
#define INPUT_RIGHT (gpio_get(PINR) == 0)
#define INPUT_DOWN (gpio_get(PIND) == 0)

#define INPUT_JX (joyStickX)
#define INPUT_JY (joyStickY)

#define INPUT_JLEFT (joyStickX < -1024)
#define INPUT_JUP (joyStickY < -1024)
#define INPUT_JRIGHT (joyStickX > 1024)
#define INPUT_JDOWN (joyStickY > 1024)

#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - 4096)
#define SETTING_CODE 0x14
#define BOOTLOADER_ENTRY_POINT (XIP_BASE + 1788000 + 0x100)
    //画面バッファ
    extern uint16_t* draw_buff;
    extern uint8_t screenStartX;
    extern uint8_t screenStartY;
    extern uint8_t screenEndX;
    extern uint8_t screenEndY;
    //制御
    void tftByte(uint8_t sbyte);
    //ビットマップ
    void drawBitmapFast(const uint16_t bmp[],int cx,int cy,uint16_t px,uint16_t py,uint16_t pw,uint16_t ph,uint16_t cw,uint16_t ch);
    void drawBitmap(const uint16_t bmp[],int cx,int cy,uint16_t px,uint16_t py,uint16_t pw,uint16_t ph,uint16_t cw,uint16_t ch);
    void drawBitmapBright(const uint16_t bmp[],int cx,int cy,uint16_t px,uint16_t py,uint16_t pw,uint16_t ph,uint16_t cw,uint16_t ch);
    //基本描画
    void clearScreen(void);
    void drawPixel(int dx,int dy,uint16_t cc);
    void drawBox(int x1,int y1,int x2,int y2,uint16_t cc);
    void drawFillBox(int x1,int y1,int x2,int y2,uint16_t cc);
    void drawLine(int x1, int y1, int x2, int y2,uint16_t cc);
    void drawString(int dx,int dy,uint16_t cc,const char str[]);
    void drawNumber(int dx, int dy, uint16_t cc,int deg);
    //色関連
    void setBright(int r,int g,int b);
    uint16_t convert16bitColor(int r,int g,int b);
    void convert24bitColor(uint16_t cc,uint8_t *r,uint8_t *g,uint8_t *b);
    //画面制御系
    void sendScreen(void);
    void tftInit(uint8_t width, uint8_t height,uint64_t freq);
    void tftScreenSettings(bool inversX,bool inversY,bool swapColor);
    void waitVsync(int frame);
    void drawFPS();
    //picoStick機能
    void startPicoStick();
    void updateJoyStick();
    extern int joyStickX;
    extern int joyStickY;
    void jump_to_app(uint32_t vtor_addr);
#endif
/*
    
    ST7735ライブラリ for Raspberry Pi Pico PICOSDK
    3shokudango
    
    2025/7/21  v1.0  新規作成
    2026/1/18  v1.1  変更点---
    バッファーを一次元配列にする
    バッファーを動的確保に変更
    転送プログラム変更
    ↑以前:2バイト→1バイトに変換して転送
    ↓現在:2バイトの配列を無理やり1バイトとして転送
    2026/02/19 v1.2  変更点
    画面の輝度設定機能追加とか文字列
    2026/02/28 v2.0 変更点
    picoStick向けに入力関数追加
    初期化一発のコマンド
    設定画面に入れる
    2026/03/06 v2.1 変更点
    外部から直接スクリーンバッファにアクセス可能
    2026/04/05 v2.2 変更点
    ブートローダーにジャンプする機能を追加
    2026/04/25 v2.2.1 変更点
    線を描画する関数を追加
    細かな修正
    ---
*/