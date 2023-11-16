/***
 * Required libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * ESP32_JPEG: https://github.com/esp-arduino-libs/ESP32_JPEG.git
 */

#define MJPEG_FILENAME "/earth.mjpeg"
#define MJPEG_BUFFER_SIZE (240 * 240 * 2 / 10)
#define MJPEG_OUTPUT_SIZE (240 * 240 * 2)

#include <WiFi.h>
#include <FS.h>

#include <FFat.h>
#include <LittleFS.h>
#include <SD.h>
#include <SD_MMC.h>
#include <SPIFFS.h>

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

// #define GFX_BL 13 // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
// Arduino_DataBus *bus = new Arduino_ESP32QSPI(10 /* CS */, 5 /* SCK */, 14 /* D0 */, 8 /* D1 */, 0 /* D2 */, 1 /* D3 */);
// Arduino_GFX *gfx = new Arduino_WEA2012(bus, 6 /* RST */);

#define GFX_BL 11 // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
Arduino_DataBus *bus = new Arduino_ESP32QSPI(3 /* CS */, 10 /* SCK */, 7 /* D0 */, 6 /* D1 */, 9 /* D2 */, 4 /* D3 */);
Arduino_GFX *gfx = new Arduino_WEA2012(bus, 2 /* RST */);
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

#include "MjpegClass.h"
static MjpegClass mjpeg;

/* variables */
static int total_frames = 0;
static unsigned long total_read_video = 0;
static unsigned long total_decode_video = 0;
static unsigned long total_show_video = 0;
static unsigned long start_ms, curr_ms;

void setup()
{
  WiFi.mode(WIFI_OFF);

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX Motion JPEG SIMD Decoder Image Viewer example");

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  if (!FFat.begin())
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
  }
  else
  {
    File mjpegFile = FFat.open(MJPEG_FILENAME, "r");

    if (!mjpegFile || mjpegFile.isDirectory())
    {
      Serial.println(F("ERROR: Failed to open " MJPEG_FILENAME " file for reading"));
      gfx->println(F("ERROR: Failed to open " MJPEG_FILENAME " file for reading"));
    }
    else
    {
      uint8_t *mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE);
      if (!mjpeg_buf)
      {
        Serial.println(F("mjpeg_buf malloc failed!"));
      }
      else
      {
        uint16_t *output_buf = (uint16_t *)malloc(MJPEG_OUTPUT_SIZE);

        Serial.println(F("MJPEG start"));

        start_ms = millis();
        curr_ms = millis();
        mjpeg.setup(
            &mjpegFile, mjpeg_buf, output_buf, true /* useBigEndian */,
            gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);

        while (mjpegFile.available() && mjpeg.readMjpegBuf())
        {
          // Read video
          total_read_video += millis() - curr_ms;
          curr_ms = millis();

          // Play video
          mjpeg.drawJpg();
          total_decode_video += millis() - curr_ms;
          curr_ms = millis();

          // gfx->draw16bitBeRGBBitmap(mjpeg.getX(), mjpeg.getY(), output_buf, 240, 240);
          gfx->draw16bitBeRGBBitmap(56, 160, output_buf, 240, 240);
          total_show_video += millis() - curr_ms;

          curr_ms = millis();
          total_frames++;
        }
        int time_used = millis() - start_ms;
        Serial.println(F("MJPEG end"));
        mjpegFile.close();
        float fps = 1000.0 * total_frames / time_used;
        Serial.printf("Total frames: %d\n", total_frames);
        Serial.printf("Time used: %d ms\n", time_used);
        Serial.printf("Average FPS: %0.1f\n", fps);
        Serial.printf("Read MJPEG: %lu ms (%0.1f %%)\n", total_read_video, 100.0 * total_read_video / time_used);
        Serial.printf("Decode video: %lu ms (%0.1f %%)\n", total_decode_video, 100.0 * total_decode_video / time_used);
        Serial.printf("Show video: %lu ms (%0.1f %%)\n", total_show_video, 100.0 * total_show_video / time_used);

        gfx->setTextSize(2);
        gfx->setTextColor(WHITE, BLACK);
        gfx->setCursor(0, 0);
        gfx->printf("\n   ESP32 JPEG SIMD Decoder\n\n");
        gfx->printf("   Frames: %d\n", total_frames);
        gfx->printf("Time used: %d ms\n", time_used);
        gfx->printf("  Avg FPS: %0.1f\n", fps);
        gfx->printf("Read File: %lu ms (%0.1f%%)\n", total_read_video, 100.0 * total_read_video / time_used);
        gfx->printf("   Decode: %lu ms (%0.1f%%)\n", total_decode_video, 100.0 * total_decode_video / time_used);
        gfx->printf("  Display: %lu ms (%0.1f%%)\n", total_show_video, 100.0 * total_show_video / time_used);
        gfx->flush();
      }
    }
  }
}

void loop()
{
}
