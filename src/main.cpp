#include <Arduino.h>
#include <GxEPD2_3C.h> 
#include "images.h"

static const uint8_t EPD_BUSY = 25;
static const uint8_t EPD_RST  = 26;
static const uint8_t EPD_DC   = 27;
static const uint8_t EPD_CS   = 15;
static const uint8_t EPD_SCK  = 13;
static const uint8_t EPD_MISO = 12;
static const uint8_t EPD_MOSI = 14;

GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT> display(GxEPD2_750c_Z08(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setup()
{
  Serial.begin(115200);
  SPI.begin(EPD_SCK, EPD_MISO, EPD_MOSI, EPD_CS);
  
  display.init(115200, true, 2, false);
  display.setRotation(0);
}

void loop()
{
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    display.drawBitmap(0, 0, bitmap_black, 800, 480, GxEPD_BLACK);

    display.drawBitmap(0, 0, bitmap_red, 800, 480, GxEPD_RED);

  } while (display.nextPage());

  Serial.println("Display finished. Sleeping...");
  display.hibernate();
  
  // 3色パネルは頻繁に書き換えると劣化するため、長めの待機時間を推奨
  delay(60000); 
}