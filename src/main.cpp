#define ZEDMD_VERSION_MAJOR 3  // X Digits
#define ZEDMD_VERSION_MINOR 2  // Max 2 Digits
#define ZEDMD_VERSION_PATCH 4  // Max 2 Digits

#ifdef ZEDMD_icn2053
  #define MATRIX_WIDTH 128
  #define MATRIX_HEIGHT 64
  #define PANELS_NUMBER 2
  #define PANEL_COUNT_WIDTH 1
  #define PANEL_COUNT_HEIGHT 1 
  // PANEL_WIDTH declared inside icn2053 library
#else
  #ifdef ZEDMD_128_64_2
      #define PANEL_WIDTH    128 // Width: number of LEDs for 1 panel.
      #define PANEL_HEIGHT   64  // Height: number of LEDs.
      #define PANELS_NUMBER  2   // Number of horizontally chained panels.
  #endif
  #ifndef PANEL_WIDTH
      #define PANEL_WIDTH    64  // Width: number of LEDs for 1 panel.
      #define PANEL_HEIGHT   32  // Height: number of LEDs.
      #define PANELS_NUMBER  2   // Number of horizontally chained panels.
  #endif
#endif

#define SERIAL_BAUD    921600  // Serial baud rate.
#ifdef ZEDMD_icn2053
#define SERIAL_TIMEOUT 8       // Time in milliseconds to wait for the next data chunk.
#else
#define SERIAL_TIMEOUT 8       // Time in milliseconds to wait for the next data chunk.
#endif
#define SERIAL_BUFFER  8192    // Serial buffer size in byte.
#define FRAME_TIMEOUT  10000   // Time in milliseconds to wait for a new frame.
#define LOGO_TIMEOUT   20000   // Time in milliseconds before the logo vanishes.


// ------------------------------------------ ZeDMD by MK47 & Zedrummer (http://ppuc.org) --------------------------------------------------
// - If you have blurry pictures, the display is not clean, try to reduce the input voltage of your LED matrix panels, often, 5V panels need
//   between 4V and 4.5V to display clean pictures, you often have a screw in switch-mode power supplies to change the output voltage a little bit
// - While the initial pattern logo is displayed, check you have red in the upper left, green in the lower left and blue in the upper right,
//   if not, make contact between the ORDRE_BUTTON_PIN (default 21, but you can change below) pin and a ground pin several times
// until the display is correct (automatically saved, no need to do it again)
// -----------------------------------------------------------------------------------------------------------------------------------------
// By pressing the RGB button while a game is running or by sending command 99,you can enable the "Debug Mode".
// The output will be:
// 000 number of frames received, regardless if any error happened
// 001 size of compressed frame if compression is enabled
// 002 size of currently received bytes of frame (compressed or decompressed)
// 003 error code if the decompression if compression is enabled
// 004 number of incomplete frames
// 005 number of resets because of communication freezes
// -----------------------------------------------------------------------------------------------------------------------------------------
// Commands:
//  2: set rom frame size as (int16) width, (int16) height
//  3: render raw data
//  6: init palette (deprectated, backward compatibility)
//  7: render 16 colors using a 4 color palette (3*4 bytes), 2 pixels per byte
//  8: render 4 colors using a 4 color palette (3*4 bytes), 4 pixels per byte
//  9: render 16 colors using a 16 color palette (3*16 bytes), 4 bytes per group of 8 pixels (encoded as 4*512 bytes planes)
// 10: clear screen
// 11: render 64 colors using a 64 color palette (3*64 bytes), 6 bytes per group of 8 pixels (encoded as 6*512 bytes planes)
// 12: handshake + report resolution, returns (int16) width, (int16) height
// 13: set serial transfer chunk size as (int8) value, the value will be multiplied with 256 internally
// 14: enable serial transfer compression
// 15: disable serial transfer compression
// 20: turn off upscaling
// 21: turn on upscaling
// 22: set brightness as (int8) value between 1 and 15
// 23: set RGB order
// 24: get brightness, returns (int8) brigtness value between 1 and 15
// 25: get RGB order, returns (int8) major, (int8) minor, (int8) patch level
// 26: turn on frame timeout
// 27: turn off frame timeout
// 30: save settings
// 31: reset
// 32: get version string, returns (int8) major, (int8) minor, (int8) patch level
// 33: get panel resolution, returns (int16) width, (int16) height
// 98: disable debug mode
// 99: enable debug mode

#define TOTAL_WIDTH (PANEL_WIDTH * PANELS_NUMBER)
#define TOTAL_WIDTH_PLANE (TOTAL_WIDTH >> 3)
#define TOTAL_HEIGHT PANEL_HEIGHT
#define TOTAL_BYTES (TOTAL_WIDTH * TOTAL_HEIGHT * 3)
#define MAX_COLOR_ROTATIONS 8
#define MIN_SPAN_ROT 60


// Pinout derived from ESP32-HUB75-MatrixPanel-I2S-DMA.h
#ifdef ZEDMD_icn2053
  #define R1_PIN  32
  #define G1_PIN  23
  #define B1_PIN  33
  #define R2_PIN  25
  #define G2_PIN  19
  #define B2_PIN  26
  #define A_PIN   27
  #define B_PIN   18
  #define C_PIN   14
  #define D_PIN   17
  #define E_PIN   15 
  #define LAT_PIN 16
  #define OE_PIN  13
  #define CLK_PIN 12

  #define ORDRE_BUTTON_PIN 21
  #define LUMINOSITE_BUTTON_PIN 22
#else
  #define R1_PIN  25
  #define G1_PIN  26
  #define B1_PIN  27
  #define R2_PIN  14
  #define G2_PIN  12
  #define B2_PIN  13
  #define A_PIN   23
  #define B_PIN   19
  #define C_PIN   5
  #define D_PIN   17
  #define E_PIN   22 // required for 1/32 scan panels, like 64x64. Any available pin would do, i.e. IO32. If 1/16 scan panels, no connection to this pin needed
  #define LAT_PIN 4
  #define OE_PIN  15
  #define CLK_PIN 16

  #define ORDRE_BUTTON_PIN 21
  #define LUMINOSITE_BUTTON_PIN 33
#endif

#include <Arduino.h>
#ifdef ZEDMD_icn2053
  #include "ESP32-HUB75-MatrixPanel-DMA.h"
#else
  #include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#endif
#include <LittleFS.h>
#include "miniz/miniz.h"
#include <Bounce2.h>

// test
#define UDPDEBUG 1
#ifdef UDPDEBUG
#include "WiFi.h"
#include <WiFiUdp.h>
WiFiUDP udp;
const char * udpAddress = "192.168.0.95";
const int udpPort = 19814;
WiFiClient wifiClient;
const char* wifihostname = "ZeDMD";
#endif

Bounce2::Button* rgbOrderButton;
Bounce2::Button* brightnessButton;

#define N_CTRL_CHARS 6
#define N_INTERMEDIATE_CTR_CHARS 4


  #ifdef UDPDEBUG
bool debugMode = true;
  #else
bool debugMode = false;
  #endif
unsigned int debugLines[6] = {0};

// !!!!! NE METTRE AUCUNE VALEURE IDENTIQUE !!!!!
unsigned char CtrlCharacters[6]={0x5a,0x65,0x64,0x72,0x75,0x6d};
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

bool lumtxt[16*5]={0,1,0,0,0,1,0,0,1,0,1,1,0,1,1,0,
                   0,1,0,0,0,1,0,0,1,0,1,0,1,0,1,0,
                   0,1,0,0,0,1,0,0,1,0,1,0,0,0,1,0,
                   0,1,0,0,0,1,0,0,1,0,1,0,0,0,1,0,
                   0,1,1,1,0,0,1,1,1,0,1,0,0,0,1,0};

unsigned char lumval[16]={0,2,4,7,11,18,30,40,50,65,80,100,125,160,200,255}; // Non-linear brightness progression

#ifdef ZEDMD_icn2053
void UDPDebug(String msg);
 unsigned long icn2053_lastrefresh=0;
 char icn2053_lastrefreshcounter=0;

  MatrixPanel_DMA *dma_display = nullptr;

  hub75_cfg_t mxconfig = {
    .mx_width = PANEL_WIDTH,
    .mx_height = PANEL_HEIGHT,
    .mx_count_width = PANEL_COUNT_WIDTH,
    .mx_count_height = PANEL_COUNT_HEIGHT,
    .gpio = {
    .r1 = R1_PIN,
    .g1 = G1_PIN,
    .b1 = B1_PIN,
    .r2 = R2_PIN,
    .g2 = G2_PIN,
    .b2 = B2_PIN,
    .a = A_PIN,
    .b = B_PIN,
    .c = C_PIN,
    .d = D_PIN,
    .e = E_PIN,
    .lat = LAT_PIN,
    .oe = OE_PIN,
    .clk = CLK_PIN,
    },
    .driver = ICN2053,
    .clk_freq = HZ_13M, 
    .clk_phase = CLK_POZITIVE,
    .color_depth = COLORx16,//PIXEL_COLOR_DEPTH
    .double_buff = DOUBLE_BUFF_ON,
    .double_dma_buff = DOUBLE_BUFF_ON,
    .decoder_INT595 = true,
  };

#else
  HUB75_I2S_CFG::i2s_pins _pins={R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
  HUB75_I2S_CFG mxconfig(
            PANEL_WIDTH,    // width
            PANEL_HEIGHT,   // height
            PANELS_NUMBER,  // chain length
            _pins           // pin mapping
            //HUB75_I2S_CFG::FM6126A  // driver chip
  );

  MatrixPanel_I2S_DMA *dma_display;
#endif

int ordreRGB[3*6]={0,1,2, 2,0,1, 1,2,0,
                   0,2,1, 1,0,2, 2,1,0};
int acordreRGB=0;

unsigned char* palette;
unsigned char* renderBuffer;
#ifdef ZEDMD_128_64_2
  uint8_t doubleBuffer[TOTAL_HEIGHT][TOTAL_WIDTH] = {0};
  uint8_t existsBuffer[TOTAL_HEIGHT][TOTAL_WIDTH / 2] = {0};
#else
  uint8_t doubleBuffer[TOTAL_BYTES] = {0};
#endif

// for color rotation
unsigned char rotCols[64];
unsigned long nextTime[MAX_COLOR_ROTATIONS];
unsigned char firstCol[MAX_COLOR_ROTATIONS];
unsigned char nCol[MAX_COLOR_ROTATIONS];
unsigned char acFirst[MAX_COLOR_ROTATIONS];
unsigned long timeSpan[MAX_COLOR_ROTATIONS];

bool mode64 = false;
bool upscaling = true;

int RomWidth=128, RomHeight=32;
int RomWidthPlane=128>>3;

unsigned char lumstep=1;

bool MireActive = false;
uint8_t displayStatus = 1;
bool handshakeSucceeded = false;
bool compression = false;
// 256 is the default buffer size of the CP210x linux kernel driver, we should not exceed it as default.
int serialTransferChunkSize = 256;
unsigned int frameCount = 0;
unsigned int errorCount = 0;
bool frameTimeout = false;
bool fastReadySent = false;

void FlashBuffer() {
  // only required for double buffer icn2053, needs around 7ms
#ifdef ZEDMD_icn2053
  unsigned long curmillis = millis();
  long dist = curmillis-icn2053_lastrefresh;
  if (dist>75) {  // long enough ago, let's draw
       dma_display->flipDMABuffer();
       UDPDebug("flash");
       icn2053_lastrefreshcounter=0;
  }
  else {
    if (++icn2053_lastrefreshcounter > 2) {
      // skip drawing to avoid timeout and hang on the Windows DMD dll
      icn2053_lastrefreshcounter=0;  // draw next frame...
      UDPDebug("skip frame");
    }
    else {
      dma_display->flipDMABuffer();
      UDPDebug("flash");
    }
  }
 icn2053_lastrefresh = curmillis;

#endif
}

#ifdef UDPDEBUG
void WifiConnect() {
  
    WiFi.setHostname(wifihostname);  
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    short counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      //Serial.print(".");
      if (++counter > 50)
        ESP.restart();
    }

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        //Serial.print(F("."));
    }
    IPAddress ip = WiFi.localIP();
    //Serial.println(F("WiFi connected"));
    //Serial.println(ip);
    UDPDebug("connected");
    
}
#endif

void sendFastReady() {
  // Indicate (R)eady, even if the frame isn't rendered yet.
  // That would allow to get the buffer filled with the next frame already.
  Serial.write('R');
  UDPDebug("sendFastReady");
  fastReadySent = true;
}

void DisplayChiffre(unsigned int chf, int x,int y,int R, int G, int B)
{
  bool min_chiffres[3*10*5] = {
    0,1,0, 0,0,1, 1,1,0, 1,1,0, 0,0,1, 1,1,1, 0,1,1, 1,1,1, 0,1,0, 0,1,0,
    1,0,1, 0,1,1, 0,0,1, 0,0,1, 0,1,0, 1,0,0, 1,0,0, 0,0,1, 1,0,1, 1,0,1,
    1,0,1, 0,0,1, 0,1,0, 0,1,0, 1,1,1, 1,1,0, 1,1,0, 0,1,0, 0,1,0, 0,1,1,
    1,0,1, 0,0,1, 1,0,0, 0,0,1, 0,0,1, 0,0,1, 1,0,1, 1,0,0, 1,0,1, 0,0,1,
    0,1,0, 0,0,1, 1,1,1, 1,1,0, 0,0,1, 1,1,0, 0,1,0, 1,0,0, 0,1,0, 1,1,0
  };

  // affiche un chiffre verticalement
  unsigned int c=chf%10;
  const int poscar=3*c;
  for (int ti=0;ti<5;ti++)
  {
    for (int tj=0;tj<4;tj++)
    {
      if (tj<3)
      {
        if (min_chiffres[poscar + tj + ti*3*10] == 1) {
          dma_display->drawPixelRGB888(x+tj, y+ti, R, G, B);
        }
        else
        {
          dma_display->drawPixelRGB888(x+tj, y+ti, 0, 0, 0);
        }
      }
      else
      {
        dma_display->drawPixelRGB888(x+tj, y+ti, 0, 0, 0);
      }
    }
  }
}

void DisplayNombre(unsigned int chf, unsigned char nc, int x, int y, int R, int G, int B)
{
  // affiche un nombre verticalement
  unsigned int acc=chf,acd=1;
  for (int ti=0;ti<(nc-1);ti++) acd*=10;
  for (int ti=0;ti<nc;ti++)
  {
    unsigned int val;
    if (nc>1) val=(unsigned int)(acc/acd); else val=chf;
    DisplayChiffre(val,x+4*ti,y,R,G,B);
    acc=acc-val*acd;
    acd/=10;
  }
}

void DisplayVersion()
{
  // display the version number to the lower left
  int ncM,ncm,ncp;
  if (ZEDMD_VERSION_MAJOR>=100) ncM=3; else if (ZEDMD_VERSION_MAJOR>=10) ncM=2; else ncM=1;
  DisplayNombre(ZEDMD_VERSION_MAJOR,ncM,4,TOTAL_HEIGHT-5,150,150,150);
  dma_display->drawPixelRGB888(4+4*ncM,TOTAL_HEIGHT-1,150,150,150);
  if (ZEDMD_VERSION_MINOR>=10) ncm=2; else ncm=1;
  DisplayNombre(ZEDMD_VERSION_MINOR,ncm,4+4*ncM+2,TOTAL_HEIGHT-5,150,150,150);
  dma_display->drawPixelRGB888(4+4*ncM+2+4*ncm,TOTAL_HEIGHT-1,150,150,150);
  if (ZEDMD_VERSION_PATCH>=10) ncp=2; else ncp=1;
  DisplayNombre(ZEDMD_VERSION_PATCH,ncp,4+4*ncM+2+4*ncm+2,TOTAL_HEIGHT-5,150,150,150);
}

void DisplayLum(void)
{
  DisplayNombre(lumstep,2,TOTAL_WIDTH/2-16/2-2*4/2+16,TOTAL_HEIGHT-5,255,255,255);
}

void DisplayText(bool* text, int width, int x, int y, int R, int G, int B)
{
  // affiche le texte "SCORE" en (x, y)
  for (unsigned int ti=0;ti<width;ti++)
  {
    for (unsigned int tj=0; tj<5;tj++)
    {
      if (text[ti+tj*width]==1) dma_display->drawPixelRGB888(x+ti,y+tj,R,G,B); else dma_display->drawPixelRGB888(x+ti,y+tj,0,0,0);
    }
  }
}

void Say(unsigned char where, unsigned int what)
{
    //DisplayNombre(where,3,0,where*5,255,255,255);
    //if (what!=(unsigned int)-1) DisplayNombre(what,10,15,where*5,255,255,255);

  #ifdef UDPDEBUG
    UDPDebug("say: "+String(where)+" - "+String(what));
  #endif
}

void ClearScreen()
{
  dma_display->clearScreen();

#ifdef ZEDMD_128_64_2
  memset(doubleBuffer, 0, TOTAL_HEIGHT * TOTAL_WIDTH);
  memset(existsBuffer, 0, TOTAL_HEIGHT * TOTAL_WIDTH / 2);
#else
  memset(doubleBuffer, 0, TOTAL_BYTES);
#endif

  #ifdef UDPDEBUG
    UDPDebug("ClearScreen");
  #endif
}

bool CmpColor(uint8_t* px1, uint8_t* px2, uint8_t colors)
{
  if (colors == 3)
  {
    return
      (px1[0] == px2[0]) &&
      (px1[1] == px2[1]) &&
      (px1[2] == px2[2]);
  }

    return px1[0] == px2[0];
}

void SetColor(unsigned char* px1, unsigned char* px2, uint8_t colors)
{
  px1[0] = px2[0];

  if (colors == 3)
  {
    px1[1] = px2[1];
    px1[2] = px2[2];
  }
}

void ScaleImage(uint8_t colors)
{
  int xoffset = 0;
  int yoffset = 0;
  int scale = 0; // 0 - no scale, 1 - half scale, 2 - double scale

  if (RomWidth == 192 && TOTAL_WIDTH == 256)
  {
    xoffset = 32;
  }
  else if (RomWidth == 192)
  {
    xoffset = 16;
    scale = 1;
  }
  else if (RomHeight == 16 && TOTAL_HEIGHT == 32)
  {
    yoffset = 8;
  }
  else if (RomHeight == 16 && TOTAL_HEIGHT == 64)
  {
    if (upscaling)
    {
      yoffset = 16;
      scale=2;
    }
    else
    {
      // Just center the DMD.
      xoffset = 64;
      yoffset = 24;
    }
  }
  else if (RomWidth == 256 && TOTAL_WIDTH == 128)
  {
    scale = 1;
  }
  else if (RomWidth == 128 && TOTAL_WIDTH == 256)
  {
    if (upscaling)
    {
      // Scaling doesn't look nice for real RGB tables like Diablo.
      scale=2;
    }
    else
    {
      // Just center the DMD.
      xoffset = 64;
      yoffset = 16;
    }
  }
  else
  {
    return;
  }

  unsigned char* panel = (unsigned char*) malloc(RomWidth * RomHeight * colors);
  memcpy(panel, renderBuffer, RomWidth * RomHeight * colors);
  memset(renderBuffer, 0, TOTAL_WIDTH * TOTAL_HEIGHT);

  if (scale == 1)
  {
    // for half scaling we take the 4 points and look if there is one colour repeated
    for (int y = 0; y < RomHeight; y += 2)
    {
      for (int x = 0; x < RomWidth; x += 2)
      {
        uint16_t upper_left = y * RomWidth * colors + x * colors;
        uint16_t upper_right = upper_left + colors;
        uint16_t lower_left = upper_left + RomWidth * colors;
        uint16_t lower_right = lower_left + colors;
        uint16_t target = (xoffset + (x/2) + (y/2) * TOTAL_WIDTH) * colors;

        // Prefer most outer upper_lefts.
        if (x < RomWidth/2)
        {
          if (y < RomHeight/2)
          {
            if (CmpColor(&panel[upper_left], &panel[upper_right], colors) || CmpColor(&panel[upper_left], &panel[lower_left], colors) || CmpColor(&panel[upper_left], &panel[lower_right], colors))
            {
              SetColor(&renderBuffer[target], &panel[upper_left], colors);
            }
            else if (CmpColor(&panel[upper_right], &panel[lower_left], colors) || CmpColor(&panel[upper_right], &panel[lower_right], colors))
            {
              SetColor(&renderBuffer[target], &panel[upper_right], colors);
            }
            else if (CmpColor(&panel[lower_left], &panel[lower_right], colors))
            {
              SetColor(&renderBuffer[target], &panel[lower_left], colors);
            }
            else
            {
              SetColor(&renderBuffer[target], &panel[upper_left], colors);
            }
          }
          else
          {
            if (CmpColor(&panel[lower_left], &panel[lower_right], colors) || CmpColor(&panel[lower_left], &panel[upper_left], colors) || CmpColor(&panel[lower_left], &panel[upper_right], colors))
            {
              SetColor(&renderBuffer[target], &panel[lower_left], colors);
            }
            else if (CmpColor(&panel[lower_right], &panel[upper_left], colors) || CmpColor(&panel[lower_right], &panel[upper_right], colors))
            {
              SetColor(&renderBuffer[target], &panel[lower_right], colors);
            }
            else if (CmpColor(&panel[upper_left], &panel[upper_right], colors))
            {
              SetColor(&renderBuffer[target], &panel[upper_left], colors);
            }
            else
            {
              SetColor(&renderBuffer[target], &panel[lower_left], colors);
            }
          }
        }
        else
        {
          if (y < RomHeight/2)
          {
            if (CmpColor(&panel[upper_right], &panel[upper_left], colors) || CmpColor(&panel[upper_right], &panel[lower_right], colors) || CmpColor(&panel[upper_right], &panel[lower_left], colors))
            {
              SetColor(&renderBuffer[target], &panel[upper_right], colors);
            }
            else if (CmpColor(&panel[upper_left], &panel[lower_right], colors) || CmpColor(&panel[upper_left], &panel[lower_left], colors))
            {
              SetColor(&renderBuffer[target], &panel[upper_left], colors);
            }
            else if (CmpColor(&panel[lower_right], &panel[lower_left], colors))
            {
              SetColor(&renderBuffer[target], &panel[lower_right], colors);
            }
            else
            {
              SetColor(&renderBuffer[target], &panel[upper_right], colors);
            }
          }
          else
          {
            if (CmpColor(&panel[lower_right], &panel[lower_left], colors) || CmpColor(&panel[lower_right], &panel[upper_right], colors) || CmpColor(&panel[lower_right], &panel[upper_left], colors))
            {
              SetColor(&renderBuffer[target], &panel[lower_right], colors);
            }
            else if (CmpColor(&panel[lower_left], &panel[upper_right], colors) || CmpColor(&panel[lower_left], &panel[upper_left], colors))
            {
              SetColor(&renderBuffer[target], &panel[lower_left], colors);
            }
            else if (CmpColor(&panel[upper_right], &panel[upper_left], colors))
            {
              SetColor(&renderBuffer[target], &panel[upper_right], colors);
            }
            else
            {
              SetColor(&renderBuffer[target], &panel[lower_right], colors);
            }
          }
        }
      }
    }
  }
  else if (scale == 2)
  {
    // we implement scale2x http://www.scale2x.it/algorithm
    uint16_t row = RomWidth * colors;
    for (int x = 0; x < RomHeight; x++)
    {
      for (int y = 0; y < RomWidth; y++)
      {
        uint8_t a[colors], b[colors], c[colors], d[colors], e[colors], f[colors], g[colors], h[colors], i[colors];
        for (uint8_t tc = 0; tc < colors; tc++)
        {
          if (y == 0 && x == 0)
          {
            a[tc] = b[tc] = d[tc] = e[tc] = panel[tc];
            c[tc] = f[tc] =                 panel[colors + tc];
            g[tc] = h[tc] =                 panel[row + tc];
            i[tc] =                         panel[row + colors + tc];
          }
          else if ((y==0) && (x==RomHeight-1))
          {
            a[tc] = b[tc] =                 panel[(x-1) * row + tc];
            c[tc] =                         panel[(x-1) * row + colors + tc];
            d[tc] = g[tc] = h[tc] = e[tc] = panel[x * row + tc];
            f[tc] = i[tc] =                 panel[x * row + colors + tc];
          }
          else if ((y==RomWidth-1) && (x==0))
          {
            a[tc] = d[tc] =                 panel[y * colors - colors + tc];
            b[tc] = c[tc] = f[tc] = e[tc] = panel[y * colors + tc];
            g[tc] =                         panel[row + y * colors - colors + tc];
            h[tc] = i[tc] =                 panel[row + y * colors + tc];
          }
          else if ((y==RomWidth-1) && (x==RomHeight-1))
          {
            a[tc] =                         panel[x * row - 2 * colors + tc];
            b[tc] = c[tc] =                 panel[x * row - colors + tc];
            d[tc] = g[tc] =                 panel[RomHeight * row - 2 * colors + tc];
            e[tc] = f[tc] = h[tc] = i[tc] = panel[RomHeight * row - colors + tc];
          }
          else if (y==0)
          {
            a[tc] = b[tc] = panel[(x-1) * row + tc];
            c[tc] =         panel[(x-1) * row + colors + tc];
            d[tc] = e[tc] = panel[x * row + tc];
            f[tc] =         panel[x * row + colors + tc];
            g[tc] = h[tc] = panel[(x+1) * row + tc];
            i[tc] =         panel[(x+1) * row + colors + tc];
          }
          else if (y == RomWidth-1)
          {
            a[tc] =         panel[x * row - 2 * colors + tc];
            b[tc] = c[tc] = panel[x * row - colors + tc];
            d[tc] =         panel[(x+1) * row - 2 * colors + tc];
            e[tc] = f[tc] = panel[(x+1) * row - colors + tc];
            g[tc] =         panel[(x+2) * row - 2 * colors + tc];
            h[tc] = i[tc] = panel[(x+2) * row - colors + tc];
          }
          else if (x==0)
          {
            a[tc] = d[tc] = panel[y * colors - colors + tc];
            b[tc] = e[tc] = panel[y * colors + tc];
            c[tc] = f[tc] = panel[y * colors + colors + tc];
            g[tc] =         panel[row + y * colors - colors + tc];
            h[tc] =         panel[row + y * colors + tc];
            i[tc] =         panel[row + y * colors + colors + tc];
          }
          else if (x==RomHeight-1)
          {
            a[tc] =         panel[(x-1) * row + y * colors - colors + tc];
            b[tc] =         panel[(x-1) * row + y * colors + tc];
            c[tc] =         panel[(x-1) * row + y * colors + colors + tc];
            d[tc] = g[tc] = panel[x * row + y * colors - colors + tc];
            e[tc] = h[tc] = panel[x * row + y * colors + tc];
            f[tc] = i[tc] = panel[x * row + y * colors + colors + tc];
          }
          else
          {
            a[tc] = panel[(x-1) * row + y * colors - colors + tc];
            b[tc] = panel[(x-1) * row + y * colors + tc];
            c[tc] = panel[(x-1) * row + y * colors + colors + tc];
            d[tc] = panel[x * row + y * colors - colors + tc];
            e[tc] = panel[x * row + y * colors + tc];
            f[tc] = panel[x * row + y * colors + colors + tc];
            g[tc] = panel[(x+1) * row + y * colors - colors + tc];
            h[tc] = panel[(x+1) * row + y * colors + tc];
            i[tc] = panel[(x+1) * row + y * colors + colors + tc];
          }
        }

        if (!CmpColor(b, h, colors) && !CmpColor(d, f, colors))
        {
          SetColor(&renderBuffer[(yoffset * TOTAL_WIDTH + x*2*TOTAL_WIDTH+y*2+xoffset) * colors], CmpColor(d, b, colors) ? d : e, colors);
          SetColor(&renderBuffer[(yoffset * TOTAL_WIDTH + x*2*TOTAL_WIDTH+y*2+1+xoffset) * colors], CmpColor(b, f, colors) ? f : e, colors);
          SetColor(&renderBuffer[(yoffset * TOTAL_WIDTH + (x*2+1)*TOTAL_WIDTH+y*2+xoffset) * colors], CmpColor(d, h, colors) ? d : e, colors);
          SetColor(&renderBuffer[(yoffset * TOTAL_WIDTH + (x*2+1)*TOTAL_WIDTH+y*2+1+xoffset) * colors], CmpColor(h, f, colors) ? f : e, colors);
        } else {
          SetColor(&renderBuffer[(yoffset * TOTAL_WIDTH + x*2*TOTAL_WIDTH+y*2+xoffset) * colors], e, colors);
          SetColor(&renderBuffer[(yoffset * TOTAL_WIDTH + x*2*TOTAL_WIDTH+y*2+1+xoffset) * colors], e, colors);
          SetColor(&renderBuffer[(yoffset * TOTAL_WIDTH + (x*2+1)*TOTAL_WIDTH+y*2+xoffset) * colors], e, colors);
          SetColor(&renderBuffer[(yoffset * TOTAL_WIDTH + (x*2+1)*TOTAL_WIDTH+y*2+1+xoffset) * colors], e, colors);
        }
      }
    }
  }
  else //offset!=0
  {
    for (int y = 0; y < RomHeight; y++)
    {
      for (int x = 0; x < RomWidth; x++)
      {
        for (uint8_t c = 0; c < colors; c++)
        {
          renderBuffer[((yoffset + y) * TOTAL_WIDTH + xoffset + x) * colors + c] = panel[(y * RomWidth + x) * colors + c];
        }
      }
    }
  }

  free(panel);
}

void DrawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
#ifdef ZEDMD_128_64_2
  uint8_t colors = ((r >> 5) << 5) + ((g >> 5) << 2) + (b >> 6);
  uint8_t colorsExist = (r ? 8 : 0) + (g ? 4 : 0) + (b ? 2 : 0) + ((r || g || b) ? 1 : 0);
  uint8_t buf = (x % 2) ? (existsBuffer[y][x / 2] >> 4) : ((existsBuffer[y][x / 2] << 4) >> 4);

  if (colors != doubleBuffer[y][x] || colorsExist != buf)
  {
    doubleBuffer[y][x] = colors;
    existsBuffer[y][x / 2] = (x % 2) ? (((existsBuffer[y][x / 2] << 4) >> 4) + (colorsExist << 4)) : (((existsBuffer[y][x / 2] >> 4) << 4) + colorsExist);
#else
  int pos = 3 * y * TOTAL_WIDTH + 3 * x;

  if (r != doubleBuffer[pos] || g != doubleBuffer[pos + 1] || b != doubleBuffer[pos + 2])
  {
    doubleBuffer[pos] = r;
    doubleBuffer[pos + 1] = g;
    doubleBuffer[pos + 2] = b;
#endif

    dma_display->drawPixelRGB888(x, y, r, g, b);
  }
}

void fillPanelRaw()
{
  int pos;

  for (int y = 0; y < TOTAL_HEIGHT; y++)
  {
    for (int x = 0; x < TOTAL_WIDTH; x++)
    {
      pos = x * 3 + y * 3 * TOTAL_WIDTH;

      DrawPixel(
        x,
        y,
        renderBuffer[pos + ordreRGB[acordreRGB * 3]],
        renderBuffer[pos + ordreRGB[acordreRGB * 3 + 1]],
        renderBuffer[pos + ordreRGB[acordreRGB * 3 + 2]]
      );
    }
  }
}

void fillPanelUsingPalette()
{
  int pos;

  for (int y = 0; y < TOTAL_HEIGHT; y++)
  {
    for (int x = 0; x < TOTAL_WIDTH; x++)
    {
      pos = y*TOTAL_WIDTH + x;

      DrawPixel(
        x,
        y,
        palette[rotCols[renderBuffer[pos]] * 3 + ordreRGB[acordreRGB * 3]],
        palette[rotCols[renderBuffer[pos]] * 3 + ordreRGB[acordreRGB * 3 + 1]],
        palette[rotCols[renderBuffer[pos]] * 3 + ordreRGB[acordreRGB * 3 + 2]]
      );
    }
  }
}

void LoadOrdreRGB()
{
  File fordre=LittleFS.open("/ordrergb.val", "r");
  if (!fordre) return;
  acordreRGB=fordre.read();
  fordre.close();
}

void SaveOrdreRGB()
{
  File fordre=LittleFS.open("/ordrergb.val", "w");
  fordre.write(acordreRGB);
  fordre.close();
}

void LoadLum()
{
  File flum=LittleFS.open("/lum.val", "r");
  if (!flum) return;
  lumstep=flum.read();
  flum.close();
}

void SaveLum()
{
  File flum=LittleFS.open("/lum.val", "w");
  flum.write(lumstep);
  flum.close();
}

void DisplayLogo(void)
{
  dma_display->setBrightness8(lumval[lumstep]);

  ClearScreen();
  LoadOrdreRGB();

  File flogo;

  if (TOTAL_HEIGHT == 64)
  {
     flogo = LittleFS.open("/logoHD.raw", "r");
  }
  else
  {
    flogo=LittleFS.open("/logo.raw", "r");
  }

  if (!flogo) {
    //Serial.println("Failed to open file for reading");
    return;
  }

  renderBuffer = (unsigned char*) malloc(TOTAL_BYTES);
  for (unsigned int tj = 0; tj < TOTAL_BYTES; tj++)
  {
    renderBuffer[tj] = flogo.read();
  }
  flogo.close();

  fillPanelRaw();

  free(renderBuffer);

  DisplayVersion();
  DisplayText(lumtxt, 16, TOTAL_WIDTH/2 - 16/2 - 2*4/2, TOTAL_HEIGHT-5, 255, 255, 255);
  DisplayLum();

  FlashBuffer();

  displayStatus = 1;
  MireActive = true;
  // Re-use this variable to save memory
  nextTime[0] = millis();

    #ifdef UDPDEBUG
    UDPDebug("DisplayLogo");
  #endif
}

void DisplayUpdate(void)
{
  ClearScreen();

  File flogo;

  if (TOTAL_HEIGHT == 64)
  {
     flogo = LittleFS.open("/ppucHD.raw", "r");
  }
  else
  {
    flogo=LittleFS.open("/ppuc.raw", "r");
  }

  if (!flogo) {
    return;
  }

  renderBuffer = (unsigned char*) malloc(TOTAL_BYTES);
  for (unsigned int tj = 0; tj < TOTAL_BYTES; tj++)
  {
    renderBuffer[tj] = flogo.read();
  }
  flogo.close();

  fillPanelRaw();
  FlashBuffer();

  free(renderBuffer);

  displayStatus = 2;
  // Re-use this variable to save memory
  nextTime[0] = millis() - (LOGO_TIMEOUT / 2);

    #ifdef UDPDEBUG
    UDPDebug("DisplayUpdate");
  #endif
}

void ScreenSaver(void)
{
  ClearScreen();
  dma_display->setBrightness8(lumval[1]);
  DisplayVersion();
  FlashBuffer();

  displayStatus = 0;

    #ifdef UDPDEBUG
    UDPDebug("Screensaver");
  #endif
}

void setup()
{
  #ifdef UDPDEBUG 
    WifiConnect();
  #endif

  rgbOrderButton = new Bounce2::Button();
  rgbOrderButton->attach(ORDRE_BUTTON_PIN, INPUT_PULLUP);
  rgbOrderButton->interval(100);
  rgbOrderButton->setPressedState(LOW);

  brightnessButton = new Bounce2::Button();
  brightnessButton->attach(LUMINOSITE_BUTTON_PIN, INPUT_PULLUP);
  brightnessButton->interval(100);
  brightnessButton->setPressedState(LOW);

#ifdef ZEDMD_icn2053
  dma_display = new MatrixPanel_DMA(mxconfig);
#else
  mxconfig.clkphase = false; // change if you have some parts of the panel with a shift
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
#endif  
  dma_display->begin();

  if (!LittleFS.begin()) {
    Say(0, 9999);
    delay(4000);
  }

  Serial.setRxBufferSize(SERIAL_BUFFER);
  Serial.setTimeout(SERIAL_TIMEOUT);
  Serial.begin(SERIAL_BAUD);
  while (!Serial);

  ClearScreen();
  LoadLum();

  #ifdef UDPDEBUG 
    if (lumstep < 12)  {
      lumstep = 15;
      acordreRGB = 1;
    }
  #endif

  dma_display->setBrightness8(lumval[lumstep]); // range is 0-255, 0 - 0%, 255 - 100%
  #ifdef UDPDEBUG
    UDPDebug("Start");
    Serial.println("start");
  #endif
  DisplayLogo();
}

bool SerialReadBuffer(unsigned char* pBuffer, unsigned int BufferSize)
{
  memset(pBuffer, 0, BufferSize);

  unsigned int transferBufferSize = BufferSize;
  unsigned char* transferBuffer;

  if (compression)
  {
    uint8_t byteArray[2];
    Serial.readBytes(byteArray, 2);
    transferBufferSize = (
      (((unsigned int) byteArray[0]) << 8) +
      ((unsigned int) byteArray[1])
    );

    transferBuffer = (unsigned char*) malloc(transferBufferSize);
  }
  else
  {
    transferBuffer = pBuffer;
  }

  if (debugMode)
  {
    Say(1, transferBufferSize);
    debugLines[1] = transferBufferSize;
  }

  // We always receive chunks of "serialTransferChunkSize" bytes (maximum).
  // At this point, the control chars and the one byte command have been read already.
  // So we only need to read the remaining bytes of the first chunk and full chunks afterwards.
  int chunkSize = serialTransferChunkSize - N_CTRL_CHARS - 1 - (compression ? 2 : 0);
  int remainingBytes = transferBufferSize;
  while (remainingBytes > 0)
  {
    int receivedBytes = Serial.readBytes(transferBuffer + transferBufferSize - remainingBytes, (remainingBytes > chunkSize) ? chunkSize : remainingBytes);
    if (debugMode)
    {
      Say(2, receivedBytes);
      debugLines[2] = receivedBytes;
    }
    if (receivedBytes != remainingBytes && receivedBytes != chunkSize)
    {
      if (debugMode)
      {
        Say(9, remainingBytes);
        Say(10, chunkSize);
        Say(11, receivedBytes);
        debugLines[4] = ++errorCount;
      }

      // Send an (E)rror signal to tell the client that no more chunks should be send or to repeat the entire frame from the beginning.
      Serial.write('E');

      return false;
    }

    // Send an (A)cknowledge signal to tell the client that we successfully read the chunk.
    Serial.write('A');
    remainingBytes -= chunkSize;
    // From now on read full amount of byte chunks.
    chunkSize = serialTransferChunkSize;
  }

  if (compression) {
    mz_ulong uncompressed_buffer_size = (mz_ulong) BufferSize;
    int status = mz_uncompress2(pBuffer, &uncompressed_buffer_size, transferBuffer, (mz_ulong*) &transferBufferSize);
    free(transferBuffer);

    if (debugMode && (MZ_OK != status))
    {
      int tmp_status = (status >= 0) ? status : (-1 * status) + 100;
      Say(3, tmp_status);
      debugLines[3] = tmp_status;
    }

    if ((MZ_OK == status) && (uncompressed_buffer_size == BufferSize)) {
      // Some HD panels take too long too render. The small ZeDMD seems to be fast enough to send a fast (R)eady signal here.
      if (TOTAL_WIDTH <= 128) {
        //sendFastReady();
      }

      if (debugMode)
      {
        Say(3, 0);
        debugLines[3] = 0;
      }

      return true;
    }

    if (debugMode && (MZ_OK == status))
    {
      // uncrompessed data isn't of expected size
      Say(3, 99);
      debugLines[3] = 99;
    }

    Serial.write('E');
    return false;
  }

  sendFastReady();
  return true;
}

void updateColorRotations(void)
{
  unsigned long actime=millis();
  bool rotfound=false;
  for (int ti=0;ti<MAX_COLOR_ROTATIONS;ti++)
  {
    if (firstCol[ti]==255) continue;
    if (actime >= nextTime[ti])
    {
        nextTime[ti] = actime + timeSpan[ti];
        acFirst[ti]++;
        if (acFirst[ti] == nCol[ti]) acFirst[ti] = 0;
        rotfound=true;
        for (unsigned char tj = 0; tj < nCol[ti]; tj++)
        {
            rotCols[tj + firstCol[ti]] = tj + firstCol[ti] + acFirst[ti];
            if (rotCols[tj + firstCol[ti]] >= firstCol[ti] + nCol[ti]) rotCols[tj + firstCol[ti]] -= nCol[ti];
        }
    }
  }
  if (rotfound==true) fillPanelUsingPalette();
}

bool wait_for_ctrl_chars(void)
{
  unsigned long ms = millis();
  unsigned char nCtrlCharFound = 0;

  while (nCtrlCharFound < N_CTRL_CHARS)
  {
    if (Serial.available())
    {
#ifdef UDPDEBUG
  unsigned char recchar = Serial.read();
  if (recchar != CtrlCharacters[nCtrlCharFound++]) {
    nCtrlCharFound = 0;
    UDPDebug("ctrl char error: "+String(recchar));
  }
#else
      if (Serial.read() != CtrlCharacters[nCtrlCharFound++]) nCtrlCharFound = 0;
#endif      
    }

    if (displayStatus == 1 && mode64 && nCtrlCharFound == 0)
    {
      // While waiting for the next frame, perform in-frame color rotations.
      UDPDebug("updateColorRotation");
      updateColorRotations();
    }
    else if (displayStatus == 3 && (millis() - nextTime[0]) > LOGO_TIMEOUT)
    {
      ScreenSaver();
    }

    // Watchdog: "reset" the communictaion if a frame timout happened.
    if (frameTimeout && handshakeSucceeded && ((millis() - ms) > FRAME_TIMEOUT))
    {
      if (debugMode)
      {
        Say(5, ++debugLines[5]);
      }
UDPDebug("Error control chars, watchdog");

      // Send an (E)rror signal.
      Serial.write('E');
      // Send a (R)eady signal to tell the client to send the next command.
      Serial.write('R');

      ms = millis();
      nCtrlCharFound = 0;
    }
  }
UDPDebug("control chars found");
  return true;
}

void loop()
{
  while (MireActive)
  {
    #ifdef UDPDEBUG   
    if (WiFi.status() != WL_CONNECTED)
      WifiConnect();
    #endif  

    rgbOrderButton->update();
    if (rgbOrderButton->pressed())
    {
      if (displayStatus != 1)
      {
        DisplayLogo();
        continue;
      }

      nextTime[0] = millis();
      acordreRGB++;
      if (acordreRGB >= 6) acordreRGB = 0;
      SaveOrdreRGB();
      fillPanelRaw();
      DisplayText(lumtxt,16,TOTAL_WIDTH/2-16/2-2*4/2,TOTAL_HEIGHT-5,255,255,255);
      DisplayLum();
      FlashBuffer();
    }

    brightnessButton->update();
    if (brightnessButton->pressed())
    {
      if (displayStatus != 1)
      {
        DisplayLogo();
        continue;
      }

      nextTime[0] = millis();
      lumstep++;
      if (lumstep>=16) lumstep=1;
      dma_display->setBrightness8(lumval[lumstep]);
      DisplayLum();
      SaveLum();
      FlashBuffer();
    }

    if (Serial.available()>0)
    {
      ClearScreen();
      FlashBuffer();
      MireActive = false;
    }
    else if ((millis() - nextTime[0]) > LOGO_TIMEOUT)
    {
      if (displayStatus == 1)
      {
        DisplayUpdate();
      }
      else if (displayStatus != 0)
      {
        ScreenSaver();
      }
    }
  }

  rgbOrderButton->update();
  if (rgbOrderButton->pressed())
  {
    debugMode = !debugMode;
  }

  // After handshake, send a (R)eady signal to indicate that a new command could be sent.
  // The client has to wait for it to avoid buffer issues. The handshake it self works without it.
  if (handshakeSucceeded) {
    if (!fastReadySent) {
      Serial.write('R');
        #ifdef UDPDEBUG
            UDPDebug("R Handshake");
      #endif
    }
    else {
      fastReadySent = false;
    }
  }

  if (wait_for_ctrl_chars())
  {
    // Updates to mode64 color rotations have been handled within wait_for_ctrl_chars(), now reset it to false.
    mode64 = false;

    unsigned char c4;
    while (Serial.available()==0);
    c4 = Serial.read();

    if (debugMode) {
      DisplayNombre(c4, 2, TOTAL_WIDTH - 3*4, TOTAL_HEIGHT - 8, 200, 200, 200);
        #ifdef UDPDEBUG
    UDPDebug("c4 Read: "+String(c4));
  #endif
    }

    if (displayStatus == 0)
    {
      // Exit screen saver.
      ClearScreen();
      dma_display->setBrightness8(lumval[lumstep]);
      displayStatus = 1;
      FlashBuffer();
    }

    switch (c4)
    {
      case 12: // ask for resolution (and shake hands)
      {
        for (int ti=0;ti<N_INTERMEDIATE_CTR_CHARS;ti++) Serial.write(CtrlCharacters[ti]);
        Serial.write(TOTAL_WIDTH&0xff);
        Serial.write((TOTAL_WIDTH>>8)&0xff);
        Serial.write(TOTAL_HEIGHT&0xff);
        Serial.write((TOTAL_HEIGHT>>8)&0xff);
        handshakeSucceeded=true;
        break;
      }

      case 2: // set rom frame size
      {
         unsigned char tbuf[4];
        if (SerialReadBuffer(tbuf, 4))
        {
          RomWidth=(int)(tbuf[0])+(int)(tbuf[1]<<8);
          RomHeight=(int)(tbuf[2])+(int)(tbuf[3]<<8);
          RomWidthPlane=RomWidth>>3;
          if (debugMode) {
            DisplayNombre(RomWidth, 3, TOTAL_WIDTH - 7*4, 4, 200, 200, 200);
            DisplayNombre(RomHeight, 2, TOTAL_WIDTH - 3*4, 4, 200, 200, 200);
          }
        }
        break;
      }

      case 13: // set serial transfer chunk size
      {
        while (Serial.available() == 0);
        int tmpSerialTransferChunkSize = ((int) Serial.read()) * 256;
        if (tmpSerialTransferChunkSize <= SERIAL_BUFFER) {
          serialTransferChunkSize = tmpSerialTransferChunkSize;
          // Send an (A)cknowledge signal to tell the client that we successfully read the chunk.
          Serial.write('A');
        }
        else {
          Serial.write('E');
        }
        break;
      }

      case 14: // enable serial transfer compression
      {
        compression = true;
        Serial.write('A');
        break;
      }

      case 15: // disable serial transfer compression
      {
        compression = false;
        Serial.write('A');
        break;
      }

      case 20: // turn off upscaling
      {
        upscaling = false;
        Serial.write('A');
        break;
      }

      case 21: // turn on upscaling
        upscaling = true;
        Serial.write('A');
        break;

      case 22: // set brightness
      {
        unsigned char tbuf[1];
        if (SerialReadBuffer(tbuf, 1))
        {
          if (tbuf[0] > 0 && tbuf[0] < 16)
          {
            lumstep = tbuf[0];
            dma_display->setBrightness8(lumval[lumstep]);
          }
          else
          {
            Serial.write('E');
          }
        }
        break;
      }

      case 23: // set RGB order
      {
        unsigned char tbuf[1];
        if (SerialReadBuffer(tbuf, 1))
        {
          if (tbuf[0] >= 0 && tbuf[0] < 6)
          {
            acordreRGB = tbuf[0];
          }
          else
          {
            Serial.write('E');
          }
        }
        break;
      }

      case 24: // get brightness
      {
        Serial.write(lumstep);
        break;
      }

      case 25: // get RGB order
      {
        Serial.write(acordreRGB);
        break;
      }

      case 26: // turn on frame timeout
      {
        frameTimeout = true;
        Serial.write('A');
      }

      case 27: // turn off frame timeout
      {
        frameTimeout = false;
        Serial.write('A');
      }

      case 30: // save settings
      {
        SaveLum();
        SaveOrdreRGB();
        Serial.write('A');
        break;
      }

      case 31: // reset
      {
        handshakeSucceeded = false;
        DisplayLogo();
        for (int ti=0;ti<64;ti++) rotCols[ti]=ti;
        Serial.write('A');
        break;
      }

      case 32: // get version
      {
        Serial.write(ZEDMD_VERSION_MAJOR);
        Serial.write(ZEDMD_VERSION_MINOR);
        Serial.write(ZEDMD_VERSION_PATCH);
        break;
      }

      case 33: // get panel resolution
      {
        Serial.write(TOTAL_WIDTH&0xff);
        Serial.write((TOTAL_WIDTH>>8)&0xff);
        Serial.write(TOTAL_HEIGHT&0xff);
        Serial.write((TOTAL_HEIGHT>>8)&0xff);
        break;
      }

      case 98: // disable debug mode
      {
        debugMode = false;
        Serial.write('A');
        break;
      }

      case 99: // enable debug mode
      {
        debugMode = true;
        Serial.write('A');
        break;
      }

      case 6: // reinit palette (deprecated)
      {
        // Just backward compatibility. We don't need that command anymore.
        Serial.write('A');
        break;
      }

      case 10: // clear screen
      {
        ClearScreen();
        displayStatus = 3;
        for (int ti=0;ti<64;ti++) rotCols[ti]=ti;
        nextTime[0] = millis();
        Serial.write('A');
        FlashBuffer();
        break;
      }

      case 3: // mode RGB24
      {
        // We need to cover downscaling, too.
        int renderBufferSize = (RomWidth < TOTAL_WIDTH || RomHeight < TOTAL_HEIGHT) ? TOTAL_BYTES : RomWidth * RomHeight * 3;
        renderBuffer = (unsigned char*) malloc(renderBufferSize);
        memset(renderBuffer, 0, renderBufferSize);

        if (SerialReadBuffer(renderBuffer, RomHeight * RomWidth * 3))
        {
          mode64=false;
          ScaleImage(3);
          fillPanelRaw();
          FlashBuffer();
        }

        free(renderBuffer);
        break;
      }

      case 8: // mode 4 couleurs avec 1 palette 4 couleurs (4*3 bytes) suivis de 4 pixels par byte
      {
        int bufferSize = 3*4 + 2*RomWidthPlane*RomHeight;
        unsigned char* buffer = (unsigned char*) malloc(bufferSize);

        if (SerialReadBuffer(buffer, bufferSize))
        {
          // We need to cover downscaling, too.
          int renderBufferSize = (RomWidth < TOTAL_WIDTH || RomHeight < TOTAL_HEIGHT) ? TOTAL_WIDTH * TOTAL_HEIGHT : RomWidth * RomHeight;
          renderBuffer = (unsigned char*) malloc(renderBufferSize);
          memset(renderBuffer, 0, renderBufferSize);
          palette = (unsigned char*) malloc(3*4);
          memset(palette, 0, 3*4);

          for (int ti = 3; ti >= 0; ti--)
          {
            palette[ti * 3] = buffer[ti*3];
            palette[ti * 3 + 1] = buffer[ti*3+1];
            palette[ti * 3 + 2] = buffer[ti*3+2];
          }
          unsigned char* frame = &buffer[3*4];
          for (int tj = 0; tj < RomHeight; tj++)
          {
            for (int ti = 0; ti < RomWidthPlane; ti++)
            {
              unsigned char mask = 1;
              unsigned char planes[2];
              planes[0] = frame[ti + tj * RomWidthPlane];
              planes[1] = frame[RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              for (int tk = 0; tk < 8; tk++)
              {
                unsigned char idx = 0;
                if ((planes[0] & mask) > 0) idx |= 1;
                if ((planes[1] & mask) > 0) idx |= 2;
                renderBuffer[(ti * 8 + tk) + tj * RomWidth]=idx;
                mask <<= 1;
              }
            }
          }
          free(buffer);

          mode64=false;
          for (int ti=0;ti<64;ti++) rotCols[ti]=ti;

          ScaleImage(1);
          fillPanelUsingPalette();
          FlashBuffer();

          free(renderBuffer);
          free(palette);
        }
        else
        {
          free(buffer);
        }
        break;
      }

      case 7: // mode 16 couleurs avec 1 palette 4 couleurs (4*3 bytes) suivis de 2 pixels par byte
      {
        int bufferSize = 3*4 + 4*RomWidthPlane*RomHeight;
        unsigned char* buffer = (unsigned char*) malloc(bufferSize);

        if (SerialReadBuffer(buffer, bufferSize))
        {
          // We need to cover downscaling, too.
          int renderBufferSize = (RomWidth < TOTAL_WIDTH || RomHeight < TOTAL_HEIGHT) ? TOTAL_WIDTH * TOTAL_HEIGHT : RomWidth * RomHeight;
          renderBuffer = (unsigned char*) malloc(renderBufferSize);
          memset(renderBuffer, 0, renderBufferSize);
          palette = (unsigned char*) malloc(48);
          memset(palette, 0, 48);

          for (int ti = 3; ti >= 0; ti--)
          {
            palette[(4 * ti + 3)* 3] = buffer[ti*3];
            palette[(4 * ti + 3) * 3 + 1] = buffer[ti*3+1];
            palette[(4 * ti + 3) * 3 + 2] = buffer[ti*3+2];
          }
          palette[0]=palette[1]=palette[2]=0;
          palette[3]=palette[3*3]/3;
          palette[4]=palette[3*3+1]/3;
          palette[5]=palette[3*3+2]/3;
          palette[6]=2*(palette[3*3]/3);
          palette[7]=2*(palette[3*3+1]/3);
          palette[8]=2*(palette[3*3+2]/3);

          palette[12]=palette[3*3]+(palette[7*3]-palette[3*3])/4;
          palette[13]=palette[3*3+1]+(palette[7*3+1]-palette[3*3+1])/4;
          palette[14]=palette[3*3+2]+(palette[7*3+2]-palette[3*3+2])/4;
          palette[15]=palette[3*3]+2*((palette[7*3]-palette[3*3])/4);
          palette[16]=palette[3*3+1]+2*((palette[7*3+1]-palette[3*3+1])/4);
          palette[17]=palette[3*3+2]+2*((palette[7*3+2]-palette[3*3+2])/4);
          palette[18]=palette[3*3]+3*((palette[7*3]-palette[3*3])/4);
          palette[19]=palette[3*3+1]+3*((palette[7*3+1]-palette[3*3+1])/4);
          palette[20]=palette[3*3+2]+3*((palette[7*3+2]-palette[3*3+2])/4);

          palette[24]=palette[7*3]+(palette[11*3]-palette[7*3])/4;
          palette[25]=palette[7*3+1]+(palette[11*3+1]-palette[7*3+1])/4;
          palette[26]=palette[7*3+2]+(palette[11*3+2]-palette[7*3+2])/4;
          palette[27]=palette[7*3]+2*((palette[11*3]-palette[7*3])/4);
          palette[28]=palette[7*3+1]+2*((palette[11*3+1]-palette[7*3+1])/4);
          palette[29]=palette[7*3+2]+2*((palette[11*3+2]-palette[7*3+2])/4);
          palette[30]=palette[7*3]+3*((palette[11*3]-palette[7*3])/4);
          palette[31]=palette[7*3+1]+3*((palette[11*3+1]-palette[7*3+1])/4);
          palette[32]=palette[7*3+2]+3*((palette[11*3+2]-palette[7*3+2])/4);

          palette[36]=palette[11*3]+(palette[15*3]-palette[11*3])/4;
          palette[37]=palette[11*3+1]+(palette[15*3+1]-palette[11*3+1])/4;
          palette[38]=palette[11*3+2]+(palette[15*3+2]-palette[11*3+2])/4;
          palette[39]=palette[11*3]+2*((palette[15*3]-palette[11*3])/4);
          palette[40]=palette[11*3+1]+2*((palette[15*3+1]-palette[11*3+1])/4);
          palette[41]=palette[11*3+2]+2*((palette[15*3+2]-palette[11*3+2])/4);
          palette[42]=palette[11*3]+3*((palette[15*3]-palette[11*3])/4);
          palette[43]=palette[11*3+1]+3*((palette[15*3+1]-palette[11*3+1])/4);
          palette[44]=palette[11*3+2]+3*((palette[15*3+2]-palette[11*3+2])/4);

          unsigned char* img=&buffer[3*4];
          for (int tj = 0; tj < RomHeight; tj++)
          {
            for (int ti = 0; ti < RomWidthPlane; ti++)
            {
              unsigned char mask = 1;
              unsigned char planes[4];
              planes[0] = img[ti + tj * RomWidthPlane];
              planes[1] = img[RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              planes[2] = img[2*RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              planes[3] = img[3*RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              for (int tk = 0; tk < 8; tk++)
              {
                unsigned char idx = 0;
                if ((planes[0] & mask) > 0) idx |= 1;
                if ((planes[1] & mask) > 0) idx |= 2;
                if ((planes[2] & mask) > 0) idx |= 4;
                if ((planes[3] & mask) > 0) idx |= 8;
                renderBuffer[(ti * 8 + tk)+tj * RomWidth]=idx;
                mask <<= 1;
              }
            }
          }
          free(buffer);

          mode64=false;
          for (int ti=0;ti<64;ti++) rotCols[ti]=ti;

          ScaleImage(1);
          fillPanelUsingPalette();
          FlashBuffer();

          free(renderBuffer);
          free(palette);
        }
        else
        {
          free(buffer);
        }
        break;
      }

      case 9: // mode 16 couleurs avec 1 palette 16 couleurs (16*3 bytes) suivis de 4 bytes par groupe de 8 points (séparés en plans de bits 4*512 bytes)
      {
        int bufferSize = 3*16 + 4*RomWidthPlane*RomHeight;
        unsigned char* buffer = (unsigned char*) malloc(bufferSize);

        if (SerialReadBuffer(buffer, bufferSize))
        {
          // We need to cover downscaling, too.
          int renderBufferSize = (RomWidth < TOTAL_WIDTH || RomHeight < TOTAL_HEIGHT) ? TOTAL_WIDTH * TOTAL_HEIGHT : RomWidth * RomHeight;
          renderBuffer = (unsigned char*) malloc(renderBufferSize);
          memset(renderBuffer, 0, renderBufferSize);
          palette = (unsigned char*) malloc(3*16);
          memset(palette, 0, 3*16);

          for (int ti = 15; ti >= 0; ti--)
          {
            palette[ti * 3] = buffer[ti*3];
            palette[ti * 3 + 1] = buffer[ti*3+1];
            palette[ti * 3 + 2] = buffer[ti*3+2];
          }
          unsigned char* img=&buffer[3*16];
          for (int tj = 0; tj < RomHeight; tj++)
          {
            for (int ti = 0; ti < RomWidthPlane; ti++)
            {
              // on reconstitue un indice à partir des plans puis une couleur à partir de la palette
              unsigned char mask = 1;
              unsigned char planes[4];
              planes[0] = img[ti + tj * RomWidthPlane];
              planes[1] = img[RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              planes[2] = img[2*RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              planes[3] = img[3*RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              for (int tk = 0; tk < 8; tk++)
              {
                unsigned char idx = 0;
                if ((planes[0] & mask) > 0) idx |= 1;
                if ((planes[1] & mask) > 0) idx |= 2;
                if ((planes[2] & mask) > 0) idx |= 4;
                if ((planes[3] & mask) > 0) idx |= 8;
                renderBuffer[(ti * 8 + tk)+tj * RomWidth]=idx;
                mask <<= 1;
              }
            }
          }
          free(buffer);

          mode64=false;
          for (int ti=0;ti<64;ti++) rotCols[ti]=ti;

          ScaleImage(1);
          fillPanelUsingPalette();
          FlashBuffer();

          free(renderBuffer);
          free(palette);
        }
        else
        {
          free(buffer);
        }
        break;
      }

      case 11: // mode 64 couleurs avec 1 palette 64 couleurs (64*3 bytes) suivis de 6 bytes par groupe de 8 points (séparés en plans de bits 6*512 bytes) suivis de 3*8 bytes de rotations de couleurs
      {
        int bufferSize = 3*64 + 6*RomWidthPlane*RomHeight + 3*MAX_COLOR_ROTATIONS;
        unsigned char* buffer = (unsigned char*) malloc(bufferSize);

        if (SerialReadBuffer(buffer, bufferSize))
        {
          // We need to cover downscaling, too.
          int renderBufferSize = (RomWidth < TOTAL_WIDTH || RomHeight < TOTAL_HEIGHT) ? TOTAL_WIDTH * TOTAL_HEIGHT : RomWidth * RomHeight;
          renderBuffer = (unsigned char*) malloc(renderBufferSize);
          memset(renderBuffer, 0, renderBufferSize);
          palette = (unsigned char*) malloc(3*64);
          memset(palette, 0, 3*64);

          for (int ti = 63; ti >= 0; ti--)
          {
            palette[ti * 3] = buffer[ti*3];
            palette[ti * 3 + 1] = buffer[ti*3+1];
            palette[ti * 3 + 2] = buffer[ti*3+2];
          }
          unsigned char* img = &buffer[3*64];
          for (int tj = 0; tj < RomHeight; tj++)
          {
            for (int ti = 0; ti < RomWidthPlane; ti++)
            {
              // on reconstitue un indice à partir des plans puis une couleur à partir de la palette
              unsigned char mask = 1;
              unsigned char planes[6];
              planes[0] = img[ti + tj * RomWidthPlane];
              planes[1] = img[RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              planes[2] = img[2*RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              planes[3] = img[3*RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              planes[4] = img[4*RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              planes[5] = img[5*RomWidthPlane*RomHeight + ti + tj * RomWidthPlane];
              for (int tk = 0; tk < 8; tk++)
              {
                unsigned char idx = 0;
                if ((planes[0] & mask) > 0) idx |= 1;
                if ((planes[1] & mask) > 0) idx |= 2;
                if ((planes[2] & mask) > 0) idx |= 4;
                if ((planes[3] & mask) > 0) idx |= 8;
                if ((planes[4] & mask) > 0) idx |= 0x10;
                if ((planes[5] & mask) > 0) idx |= 0x20;
                renderBuffer[(ti * 8 + tk)+tj * RomWidth]=idx;
                mask <<= 1;
              }
            }
          }
          img = &buffer[3*64 + 6*RomWidthPlane*RomHeight];
          unsigned long actime = millis();
          for (int ti=0;ti<64;ti++) rotCols[ti]=ti;
          for (int ti=0;ti<MAX_COLOR_ROTATIONS;ti++)
          {
            firstCol[ti]=img[ti*3];
            nCol[ti]=img[ti*3+1];
            // acFirst[ti]=0;
            timeSpan[ti]=10*img[ti*3+2];
            if (timeSpan[ti]<MIN_SPAN_ROT) timeSpan[ti]=MIN_SPAN_ROT;
            nextTime[ti]=actime+timeSpan[ti];
          }
          free(buffer);

          mode64=true;

          ScaleImage(1);
          fillPanelUsingPalette();
          FlashBuffer();

          free(renderBuffer);
          free(palette);
        }
        else
        {
          free(buffer);
        }
        break;
      }

      default:
      {
        Serial.write('E');
      }
    }

    if (debugMode)
    {
      DisplayNombre(RomWidth, 3, TOTAL_WIDTH - 7*4, 4, 200, 200, 200);
      DisplayNombre(RomHeight, 2, TOTAL_WIDTH - 3*4, 4, 200, 200, 200);
      DisplayNombre(c4, 2, TOTAL_WIDTH - 3*4, TOTAL_HEIGHT - 8, 200, 200, 200);

      // An overflow of the unsigned int counters should not be an issue, they just reset to 0.
      debugLines[0] = ++frameCount;
      for (int i = 0; i < 6; i++)
      {
        Say((unsigned char) i, debugLines[i]);
      }
    }
  }
}




void UDPDebug(String message) {
#ifdef UDPDEBUG
  udp.beginPacket(udpAddress, udpPort);
  udp.write((const uint8_t* ) message.c_str(), (size_t) message.length());
  udp.endPacket();
#endif  
}

void UDPDebug(const char * message) {
  #ifdef UDPDEBUG
  udp.beginPacket(udpAddress, udpPort);
  udp.write((const uint8_t*) message, strlen(message));
  udp.endPacket();
  #endif  
  }

