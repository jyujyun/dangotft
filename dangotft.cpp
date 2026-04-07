#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#include "dangotft.h"
#include "font.h"
#define SPI_PORT spi0
#define PIN_MISO 4
#define PIN_CS   5
#define PIN_SCK  2
#define PIN_MOSI 3

#define RST 7  //Reset
#define DC 6   //Data Command

#define TCOLOR 0xF81F //透明色

uint8_t screenWidth;
uint8_t screenHeight;
uint16_t* draw_buff;
uint8_t* tftbits;

uint8_t screenStartX = 0;
uint8_t screenStartY = 0;
uint8_t screenEndX = 0;
uint8_t screenEndY = 0;


uint8_t colorR,colorG,colorB;
//1バイト送る
void tftByte(uint8_t sbyte)
{
  //ハードウェアSPI
  spi_write_blocking(SPI_PORT, &sbyte, 1);
}
//コマンド送る
void tftCommand(uint8_t sbyte)
{
  gpio_put(DC,0);
  tftByte(sbyte);
  gpio_put(DC,1);
}
//色を送る
void tftColor16(int cc)
{
  tftByte(cc >> 8);
  tftByte(cc & 0x00FF);
}
//サイズと座標設定
void tftSetpos(uint8_t cx,uint8_t cy,uint8_t cw,uint8_t ch)
{
  gpio_put(DC,0);
  tftByte(0x2A);    //X軸設定
  gpio_put(DC,1);
  tftByte(0);
  tftByte(cx + screenStartX);      //開始場所
  tftByte(0);
  tftByte(cx + cw + screenEndX); //終了場所
  gpio_put(DC,0);
  tftByte(0x2B);    //Y軸設定
  gpio_put(DC,1);
  tftByte(0);
  tftByte(cy + screenStartY);      //開始場所
  tftByte(0);
  tftByte(cy + ch + screenEndY); //終了場所
  gpio_put(DC,0);
}
//画面の輝度設定 それぞれの明るさ
void setBright(int r,int g,int b)
{
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  colorR = r;
  colorG = g;
  colorB = b;
}
void convert24bitColor(uint16_t cc,uint8_t *r,uint8_t *g,uint8_t *b)
{
  *r = (cc >> 11) << 3;
  *g = ((cc >> 5) & 0b0000000000111111) << 2;
  *b = (cc & 0b0000000000011111) << 3;
}
uint16_t convert16bitColor(uint8_t r,uint8_t g,uint8_t b)
{
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}
//輝度に応じた色にする
uint16_t getBrightColor(uint16_t cc)
{
  if (colorR < 255 || colorG < 255 || colorB < 255)
  {
    uint8_t cR = (cc >> 11);
    uint8_t cG = (cc >> 5) & 0b0000000000111111;
    uint8_t cB = cc & 0b0000000000011111;
    cR = (cR * colorR) >> 8;
    cG = (cG * colorG) >> 8;
    cB = (cB * colorB) >> 8;
    cc = ((cR << 11) | (cG << 5) | cB);
    return cc;
  }
  return cc;
}
//cx,cy:描画先座標
//px,py:画像の切り出し開始座標
//pw,ph:切り出しサイズ
//cw,ch:画像全体サイズ
void drawBitmapFast(const uint16_t  bmp[],int cx,int cy,uint16_t px,uint16_t py,uint16_t pw,uint16_t ph,uint16_t cw,uint16_t ch)
{
  for (int j = 0;j < ph;j ++)
  {
      memcpy(&draw_buff[cx + ((j + cy) * screenWidth)],&bmp[px + ((py + j) * cw)],pw * 2);
  }
}
void drawBitmap(const uint16_t  bmp[],int cx,int cy,uint16_t px,uint16_t py,uint16_t pw,uint16_t ph,uint16_t cw,uint16_t ch)
{
  for (int j = 0;j < ph;j ++)
  {
    for (int i = 0;i < pw; i ++)
    {
      if (i + cx >= 0 && i + cx < screenWidth && j + cy >= 0 && j + cy < screenHeight)
      {
          uint16_t bmpC = bmp[px + i + ((py + j) * cw)];
          if (bmpC != TCOLOR)
          {
            draw_buff[i + cx + (j + cy) * screenWidth] = bmpC;
          }
      }
    }
  }
}
void drawBitmapBright(const uint16_t  bmp[],int cx,int cy,uint16_t px,uint16_t py,uint16_t pw,uint16_t ph,uint16_t cw,uint16_t ch)
{
  for (int j = 0;j < ph;j ++)
  {
    for (int i = 0;i < pw; i ++)
    {
      if (i + cx >= 0 && i + cx < screenWidth && j + cy >= 0 && j + cy < screenHeight)
      {
          uint16_t bmpC = bmp[px + i + ((py + j) * cw)];
          if (bmpC != TCOLOR)
          {
            bmpC = getBrightColor(bmpC);
            draw_buff[i + cx + (j + cy) * screenWidth] = bmpC;
          }
      }
    }
  }
}
void drawPixel(int dx,int dy,uint16_t cc)
{
  if (dx >= 0 && dx < screenWidth && dy >= 0 && dy < screenHeight)
  {
    cc = getBrightColor(cc);
    draw_buff[dy * screenWidth + dx] = cc;
  }
}
void drawFillBox(int x1,int y1,int x2,int y2,uint16_t cc)
{
  cc = getBrightColor(cc);
  for (int i = x1;i < x2;i ++)
  {
    for (int j = y1;j < y2;j ++)
    {
     if (i >= 0 && i < screenWidth && j >= 0 && j < screenHeight)
      {
        draw_buff[i + j * screenWidth] = cc;
      }   
    } 
  }
}
void drawBox(int x1,int y1,int x2,int y2,uint16_t cc)
{
  cc = getBrightColor(cc);
  for (int i = x1; i < x2;i ++)
  {
    draw_buff[i + y1 * screenWidth] = cc;
    draw_buff[i + (y2 - 1) * screenWidth] = cc;
  }
  for (int j = y1; j < y2;j ++)
  {
      draw_buff[x1 + j * screenWidth] = cc;
      draw_buff[(x2 - 1) + j * screenWidth] = cc;
  }
}
void drawLine(int x1, int y1, int x2, int y2,uint16_t cc) 
{
  int dx = abs(x2-x1), sx = x1<x2 ? 1 : -1;
  int dy = abs(y2-y1), sy = y1<y2 ? 1 : -1;
  int err = (dx>dy ? dx : -dy)/2, e2;
  while(1)
  {
     drawPixel(x1,y1,cc);
     if (x1==x2 && y1==y2) break;
     e2 = err;
     if (e2 >-dx) { err -= dy; x1 += sx; }
     if (e2 < dy) { err += dx;y1 += sy; }
  }
}
//文字
void drawCharSSD1306(int dx,int dy,int code,uint16_t cr,const uint8_t fontB[],uint8_t fontW)
{
  for (int i = 0;i < fontW;i ++)
  {
    for (int j = 0;j < 8;j ++)
    {
      if (fontB[code * fontW + i] & (1 << j))
      {
        draw_buff[dx + i + ((dy + j) * screenWidth)] = cr;
      }
    }
  }
}
void drawNumber(int dx, int dy, uint16_t cc,int deg)
{
  cc = getBrightColor(cc);
  if (deg < 0)
  {
    drawCharSSD1306(dx, dy, '-' - 32, cc, font6x8E, 6);
    deg = -deg;
    dx += 6;
  }
    int countDeg = 1;
    while (1)
    {
        //桁数(1,10,100..を求める)
        if (deg < countDeg * 10) { break; }
        else { countDeg *= 10; }
    }
    while (1)
    {
      drawCharSSD1306(dx, dy, ((deg / countDeg) % 10) + 16, cc, font6x8E, 6);
        if (countDeg >= 10)
        {
            dx += 6;
            countDeg /= 10;
        }
        else { break; }
    }
}
void drawString(int dx,int dy,uint16_t cc,const char str[])
{
	int strC = 0;
  cc = getBrightColor(cc);
	while (1) {
    if (str[strC] == '\n')
    {
      dy += 8;
      strC += 1;
    }
		else if (str[strC] == 227)
		{
			//ひらがなデコード
			uint16_t drawAdd = ((uint8_t)str[strC + 1] << 8);
			drawAdd |= str[strC + 2];
			drawAdd -= 33153;
			if (drawAdd >= 0xFF)
			{
				drawAdd -= 0xC0;
			}
			drawCharSSD1306(dx,dy,drawAdd,cc,font_jp_8,8);
			strC += 3;
			dx += 8;
		}
		else if (str[strC] >= 228)
		{
			//漢字でコード
			uint16_t cByte[3];
            cByte[0] = str[strC] & 0b00001111;
            cByte[1] = str[strC + 1] & 0b00111111;
            cByte[2] = str[strC + 2] & 0b00111111;
            uint16_t mojiCode;
			
            mojiCode = cByte[0] << 12 | cByte[1] << 6 | cByte[2];
			uint16_t drawAdd = shiftJIS[mojiCode - 0x4E00];
			drawCharSSD1306(dx,dy,drawAdd,cc,kanji,8);
			strC += 3;
			dx += 8;
		}
		else
		{
			int drawAdd = str[strC] - 32; 
			drawCharSSD1306(dx,dy,drawAdd,cc,font6x8E,6);
			strC += 1;
			dx += 6;
		}

		if (str[strC] == 0)
		{
			break;
		}
	}
}

void waitVsync(int frame)
{
  static uint64_t lastSec = 0;
  int limit = (1000000 / frame);
  while (time_us_64() - lastSec <= limit){}
  lastSec = time_us_64();
}
void drawFPS()
{
  static int frame = 0;
  static int nFrame = 0;
  static uint64_t next = 0;

  uint64_t now = time_us_64();

  if (next == 0)
  {
    next = now + 1000000ULL;
  }
  frame++;

  if (now >= next)
  {
    nFrame = frame;
    frame = 0;
    next += 1000000ULL;
  }

  drawNumber(0,0,0xFFFF,nFrame);
}

//画面クリア
void clearScreen(void)
{
  for (int i = 0;i < screenWidth * screenHeight;i ++)
  {
    draw_buff[i] = 0x0000;
  }
}
int spi_dma_chan;
dma_channel_config spi_dma_cfg;
void dmaInit(void)
{
    spi_dma_chan = dma_claim_unused_channel(true);
    spi_dma_cfg = dma_channel_get_default_config(spi_dma_chan);
    // 8bit転送
    channel_config_set_transfer_data_size(&spi_dma_cfg, DMA_SIZE_8);
    // 読み元はインクリメント（バッファ）
    channel_config_set_read_increment(&spi_dma_cfg, true);
    // 書き先は固定（SPIデータレジスタ）
    channel_config_set_write_increment(&spi_dma_cfg, false);
    // SPI TX要求で転送
    channel_config_set_dreq(&spi_dma_cfg, DREQ_SPI0_TX);
    dma_channel_configure(
        spi_dma_chan,
        &spi_dma_cfg,
        &spi_get_hw(spi0)->dr, // 書き先
        NULL,                  // 読み元（後で指定）
        0,                     // 転送数（後で指定）
        false                  // 今は開始しない
    );
}
void dmaSend(uint8_t *data, uint32_t length)
{
    // DMAが終わるまで待つ（ダブルバッファなら不要な場合あり）
    dma_channel_wait_for_finish_blocking(spi_dma_chan);
    // 読み元と転送数を設定
    dma_channel_set_read_addr(spi_dma_chan, data, false);
    dma_channel_set_trans_count(spi_dma_chan, length, true);
}
void sendScreen(void )
{
  gpio_put(DC,0); 
  tftSetpos(0,0,screenWidth,screenHeight);
  
  gpio_put(DC,0);
  tftByte(0x2C);
  gpio_put(DC,1);
  
  for (int i = 0;i < screenWidth * screenHeight;i ++)
  {
    tftbits[i * 2] = draw_buff[i] >> 8;
    tftbits[i * 2 + 1] = draw_buff[i] & 0x00FF;
  }
  //spi_write_blocking(SPI_PORT, tftbits, screenWidth * screenHeight * 2);
  dmaSend(tftbits, screenWidth * screenHeight * 2);
}
void tftScreenSettings(bool inversX,bool inversY,bool swapColor)
{
  //画面設定
  gpio_put(DC,0);
  tftByte(0x36);
  gpio_put(DC,1);
  uint8_t setting = 0b00110000;
  if (inversX)
  {
    setting |= 0b01000000;
  }
  if (inversY)
  {
    setting |= 0b10000000;
  }
  if (swapColor)
  {
    setting |= 0b00000100;
  }
  tftByte(setting);
  gpio_put(DC,0);
}
void tftInit(uint8_t width, uint8_t height,uint64_t freq)
{
    screenWidth = width;
    screenHeight = height;
    colorR = 255;
    colorG = 255;
    colorB = 255;
    draw_buff = (uint16_t*)malloc(screenWidth * screenHeight * sizeof(uint16_t));
    tftbits = (uint8_t*)malloc(screenWidth * screenHeight * 2);
    //spi_init(SPI_PORT, 62500000);  //周波数フルスロットル!
    spi_init(SPI_PORT, freq);  //周波数オーバークロック!
    dmaInit();
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(RST);
    gpio_init(DC);

    gpio_set_dir(RST, GPIO_OUT);
    gpio_set_dir(DC, GPIO_OUT);

      gpio_put(RST,1);
  sleep_ms(300);
  gpio_put(RST,0);
  sleep_ms(100);
  gpio_put(RST,1);
  sleep_ms(300);
  //SPIつうしんかいし
  tftCommand(0x01); //ソフトウェアリセット
  sleep_ms(500);
  tftCommand(0x28); //画面オフ
  tftCommand(0x11); //スリープ解除

  //色設定
  gpio_put(DC,0);
  tftByte(0x3A);
  gpio_put(DC,1);
  tftByte(0x5);
  gpio_put(DC,0);


  //画面設定
  gpio_put(DC,0);
  tftByte(0x36);
  gpio_put(DC,1);
//tftByte(0b01101000);
  tftByte(0b10101010);
  gpio_put(DC,0);
  
  /*Pガンマ*/
  gpio_put(DC,0);
  tftByte(0xE0);
  gpio_put(DC,1);
  tftByte(0x09);
  tftByte(0x16);
  tftByte(0x09);
  tftByte(0x20);
  tftByte(0x21);
  tftByte(0x1B);
  tftByte(0x13);
  tftByte(0x19);
  tftByte(0x17);
  tftByte(0x15);
  tftByte(0x1E);
  tftByte(0x2B);
  tftByte(0x04);
  tftByte(0x05);
  tftByte(0x02);
  tftByte(0x0E);
  gpio_put(DC,0);

  /*Nガンマ*/
  gpio_put(DC,0);
  tftByte(0xE1);
  gpio_put(DC,1);
  tftByte(0x0B);
  tftByte(0x14);
  tftByte(0x08);
  tftByte(0x1E);
  tftByte(0x22);
  tftByte(0x1D);
  tftByte(0x18);
  tftByte(0x1E);
  tftByte(0x1B);
  tftByte(0x1A);
  tftByte(0x24);
  tftByte(0x2B);
  tftByte(0x06);
  tftByte(0x06);
  tftByte(0x02);
  tftByte(0x0F);
  gpio_put(DC,0);
  
  //ふつうモード
  tftCommand(0x13);
  tftCommand(0x20);



  tftCommand(0x29);  //画面オン

  sleep_ms(100);
}
#define MASTER_CLOCK_FREQ 166000000

int joyStickX = 0;
int joyStickY = 0;
void updateJoyStick()
{
  adc_select_input(0);         // ADC0（GPIO26）を選択
  joyStickX = adc_read() - 2048;
  adc_select_input(1);         // ADC0（GPIO26）を選択
  joyStickY = adc_read() - 2048;
}
//ブートローダーへジャンプ
void jump_to_app(uint32_t vtor_addr) {
    // 割り込みを完全に無効化
    //__disable_irq();

    // VTORをメインアプリのものに切り替え
    scb_hw->vtor = vtor_addr;

    // スタックポインタ(SP)とプログラムカウンタ(PC)を取得
    uint32_t sp = *(uint32_t *)vtor_addr;
    uint32_t pc = *(uint32_t *)(vtor_addr + 4);

    // スタックポインタを再設定してジャンプ
    asm volatile (
        "msr msp, %0 \n"
        "bx %1       \n"
        : : "r" (sp), "r" (pc)
    );
}
//ゲーム機初期化
void startPicoStick()
{
  //マスタクロック設定
    set_sys_clock_khz(MASTER_CLOCK_FREQ / 1000, true);
        clock_configure(
        clk_peri,
        0,
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
        MASTER_CLOCK_FREQ,
        MASTER_CLOCK_FREQ
    );
    //初期化
    stdio_init_all();

    //GPIO設定
    gpio_init(PINL);
    gpio_set_dir(PINL, GPIO_IN);
    gpio_pull_up(PINL);

    gpio_init(PINU);
    gpio_set_dir(PINU, GPIO_IN);
    gpio_pull_up(PINU);

    gpio_init(PINR);
    gpio_set_dir(PINR, GPIO_IN);
    gpio_pull_up(PINR);

    gpio_init(PIND);
    gpio_set_dir(PIND, GPIO_IN);
    gpio_pull_up(PIND);

    adc_init();
    adc_gpio_init(PINJX);
    adc_gpio_init(PINJY);

    //上キーが押された場合ブートローダへジャンプ
    if (INPUT_UP)
    {
      jump_to_app(0x101BF100);
    }
    //設定読み出し 下ボタンが押されていた場合バイパス
    if (!INPUT_DOWN)
    {
      const uint8_t *loadSettings = (const uint8_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
      screenStartX = loadSettings[1];
      screenEndX = loadSettings[2];
      screenStartY = loadSettings[3];
      screenEndY = loadSettings[4];
      //0xFFなどの場合(多くの場合未使用)上書き
      if (screenStartX >= 64)
      {
        uint32_t ints = save_and_disable_interrupts();
        uint8_t saveSettings[256];
        saveSettings[0] = SETTING_CODE;
        saveSettings[1] = 0;
        saveSettings[2] = 0;
        saveSettings[3] = 0;
        saveSettings[4] = 0;
        flash_range_erase(FLASH_TARGET_OFFSET, 4096);
        flash_range_program(FLASH_TARGET_OFFSET, saveSettings, 256);
        restore_interrupts(ints);
      }
    }
    //スクリーン初期化
    tftInit(160,128,MASTER_CLOCK_FREQ / 4);
    tftScreenSettings(false,true,false);
    waitVsync(60);

}
