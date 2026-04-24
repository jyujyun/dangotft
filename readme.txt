dangoTFTの説明

RaspberryPiPico、RP2040、PicoSDK用のプログラム
ST7735を制御可能

*このプログラムは私のプロジェクトであるゲーム機用に作られています


[特徴]
-四角形や透過ビットマップなどの描画関数
-内部バッファ方式

[使用方法]
1.PicoSDKをダウンロードする
2.dangotft.cppとdangotft.hとfont.hをプロジェクトのフォルダーの中に入れる
3.CMakeLists.txtのtarget_link_libraryに
        hardware_spi
        hardware_clocks
        hardware_irq
        hardware_dma
        hardware_adc
        hardware_flash
        hardware_sync
        pico_rand
を追加する
4.CMakeLists.txtにadd_executableにdangotft.cppを追加する

これで準備は整いました。

[主な関数一覧]
ここに書いていない関数もあります。
色はすべて16ビットカラー(uint16_t)です。
PicoStickで使う方は(準備中)を見たほうがいいですよ。

-画面基本制御系
void startPicoStick(void)
PicoStickのリセットを行います。

void sendScreen(void)
画面の情報をマイコンからST7735に転送します。
これを実行しない限り画面の内容は更新されません。

-基本描画系
void sendScreen(void)
画面の内容をすべて消します。

void drawPixel(x,y,color)
点を打ちます。

void drawBox(x1,y1,x2,y2,color)
四角形を描画します。

void drawFillBox(x1,y1,x2,y2,color)
塗りつぶされた四角形を描画します。

void drawstring(x,y,color,string)
文字列を描画します。日本語可能。

void drawLine(x1,y1,x2,y2,color)
直線を描画します。

-ビットマップ
void drawBitmapBright(bmp,x,y,startX,startY,width,height,imageWidth,imageHeight)
x,yにビットマップを描画します。startX,startYは切り取る始点、width,heightは横幅と高さ、imageWidthとimageHeightは画像自体の横幅と高さです。

void drawBitmap(bmp,x,y,startX,startY,width,height,imageWidth,imageHeight)
上の関数と同じですあが、setBrightの効果を受けません。

void drawBitmapFast(bmp,x,y,startX,startY,width,height,imageWidth,imageHeight)
上の関数と同じですが、制約が多い代わりに高速で描画ができます。
透明色の描画不可、範囲外の描画不可(画面の範囲から画像がはみ出すとアウト)
今後改善予定です。

-その他関数
void setBright(r,g,b)
この関数が実行される以降に描画される色の強さを変えます。


-以下はpicoStick(私のプロジェクト)以外で実行する時以外で使います。
void tftInit(width,height,freq)
ST7735の初期化を開始します。
freqはSPI通信の周波数です。
