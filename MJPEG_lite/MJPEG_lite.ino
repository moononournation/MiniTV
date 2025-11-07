/*******************************************************************************
 * MJPEG Player example
 *
 * Dependent libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 *
 * Setup steps:
 * 1. Change your LCD parameters in Arduino_GFX setting
 * 2. Upload AVI file
 *   FFat/LittleFS:
 *     upload FFat (FatFS) data with ESP32 Sketch Data Upload:
 *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
 *   SD:
 *     Copy files to SD card
 ******************************************************************************/
#define MJPEG_FILENAME "/output.mjpeg"
#define GFX_BRIGHTNESS 239

// Dev Device Pins: <https://github.com/moononournation/Dev_Device_Pins.git>
// #include "PINS_ESP32-C6-LCD-1_47.h"
// #include "PINS_ESP32-S3-LCD-1_47.h"
#include "PINS_RP2350-LCD-1_47.h"

#if defined(RGB_PANEL) || defined(DSI_PANEL) || defined(CANVAS)
// use little endian pixel
#else
#define BIG_ENDIAN_PIXEL
#endif
#include "MJPEG.h"

#ifdef ESP32
#include <FFat.h>
#include <SPIFFS.h>
#endif
#include <LittleFS.h>
#include <SD.h>
#ifdef SOC_SDMMC_HOST_SUPPORTED
#include <SD_MMC.h>
#endif

/* variables */
int total_frames;
unsigned long total_decode_video;
unsigned long total_show_video;
unsigned long start_ms;

int JPEGDraw(JPEGDRAW *pDraw)
{
  // Serial.printf("Draw pos = (%d, %d), size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  unsigned long s = millis();
#ifdef BIG_ENDIAN_PIXEL
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
#else
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
#endif
  total_show_video += millis() - s;
  return 1;
}

void setup()
{
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("MJPEG Player Callback");

  // If display and SD shared same interface, init SPI first
#ifdef SPI_SCK
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
#endif

  // Init Display
  if (!gfx->begin(GFX_SPEED))
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);
#if defined(RGB_PANEL) || defined(DSI_PANEL) || defined(CANVAS)
  gfx->flush(true /* force_flush */);
#endif

#ifdef GFX_BL
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR < 3)
  ledcSetup(0, 1000, 8);
  ledcAttachPin(GFX_BL, 0);
  ledcWrite(0, GFX_BRIGHTNESS);
#elif defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcAttachChannel(GFX_BL, 1000, 8, 1);
  ledcWrite(GFX_BL, GFX_BRIGHTNESS);
#else
  pinMode(GFX_BL, OUTPUT);
  analogWrite(GFX_BL, GFX_BRIGHTNESS);
#endif
#endif // GFX_BL

#ifdef ESP32
#if defined(SD_D1) && defined(SOC_SDMMC_HOST_SUPPORTED)
#define FILESYSTEM SD_MMC
#define OPENFS mjpegOpenSD_MMC
  SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */, SD_D1, SD_D2, SD_CS /* D3 */);
  if (!SD_MMC.begin("/root", false /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
#elif defined(SD_SCK) && defined(SOC_SDMMC_HOST_SUPPORTED)
#define FILESYSTEM SD_MMC
#define OPENFS mjpegOpenSD_MMC
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */);
  if (!SD_MMC.begin("/root", true /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
#elif defined(SD_CS)
#define FILESYSTEM SD
#define OPENFS mjpegOpenSD
  if (!SD.begin(SD_CS, SPI, 80000000, "/root"))
#else
// #define FILESYSTEM FFat
#define FILESYSTEM LittleFS
// #define FILESYSTEM SPIFFS
#define OPENFS mjpegOpenLittleFS
  if (!FILESYSTEM.begin(false, "/root"))
#endif
#else // !ESP32
#ifdef SD_D1
#define FILESYSTEM SD
#define OPENFS mjpegOpenSD
  if (!SD.begin(SD_SCK, SD_MOSI, SD_MISO))
#else
#define FILESYSTEM LittleFS
#define OPENFS mjpegOpenLittleFS
  if (!FILESYSTEM.begin())
#endif
#endif
  {
    Serial.println("ERROR: File system mount failed!");
    gfx->println("ERROR: File System Mount Failed!");
    delay(INT_MAX);
  }
}

void loop()
{
  Serial.println(F("MJPEG start"));

  start_ms = millis();
  total_frames = 0;
  total_decode_video = 0;
  total_show_video = 0;
  isMjpegEnded = false;

  while (!isMjpegEnded)
  {
    /* select single play or loopback forever */
    // if (jpeg.open(MJPEG_FILENAME, OPENFS, mjpegClose, mjpegRead, mjpegSeek, JPEGDraw))
    if (jpeg.open(MJPEG_FILENAME, OPENFS, mjpegCloseLoopback, mjpegRead, mjpegSeek, JPEGDraw))
    {
#ifdef BIG_ENDIAN_PIXEL
      jpeg.setPixelType(RGB565_BIG_ENDIAN);
#else
      jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
#endif
      jpeg.decode(0 /* x */, 0 /* y */, 0 /* options */);
      jpeg.close();
      ++total_frames;
    }
  }

  int time_used = millis() - start_ms;
  Serial.println("MJPEG end");
  float fps = 1000.0 * total_frames / time_used;
  total_decode_video = time_used - total_show_video;
  Serial.printf("Total frames: %d\n", total_frames);
  Serial.printf("Time used: %d ms\n", time_used);
  Serial.printf("Average FPS: %0.1f\n", fps);
  Serial.printf("Decode video: %lu ms (%0.1f %%)\n", total_decode_video, 100.0 * total_decode_video / time_used);
  Serial.printf("Show video: %lu ms (%0.1f %%)\n", total_show_video, 100.0 * total_show_video / time_used);
  delay(INT_MAX);
}
