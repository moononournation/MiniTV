/*******************************************************************************
 * MJPEG Player example
 *
 * Dependent libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * ESP32_JPEG: https://github.com/esp-arduino-libs/ESP32_JPEG.git
 *
 * Setup steps:
 * 1. Change your LCD parameters in Arduino_GFX setting
 * 2. Upload Motion JPEG file
 *   FFat/LittleFS:
 *     upload FFat (FatFS) data with ESP32 Sketch Data Upload:
 *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
 *   SD:
 *     Copy files to SD card
 ******************************************************************************/
#define ROOT "/root"
#define MJPEG_FILENAME ROOT "/output.mjpeg"
#define MJPEG_OUTPUT_SIZE (320 * 480 * 2)          // memory for a output image frame
#define MJPEG_BUFFER_SIZE (MJPEG_OUTPUT_SIZE / 4) // memory for a single JPEG frame
#define GFX_BRIGHTNESS 239

// Dev Device Pins: <https://github.com/moononournation/Dev_Device_Pins.git>
#include "PINS_JC3248W535.h"

#if defined(RGB_PANEL) || defined(DSI_PANEL) || defined(CANVAS)
// use little endian pixel
#else
#define BIG_ENDIAN_PIXEL
#endif
#include "MjpegClass.h"

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
MjpegClass mjpeg;
int total_frames;
unsigned long total_read_video;
unsigned long total_decode_video;
unsigned long total_show_video;
unsigned long start_ms, curr_ms;
long output_buf_size, estimateBufferSize;
uint8_t *mjpeg_buf;
uint16_t *output_buf;

int16_t x = -1, y = -1, w = -1, h = -1;

void setup()
{
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("MJPEG Player SIMD");

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

#if defined(SD_D1) && defined(SOC_SDMMC_HOST_SUPPORTED)
#define FILESYSTEM SD_MMC
  SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */, SD_D1, SD_D2, SD_CS /* D3 */);
  if (!SD_MMC.begin(ROOT, false /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
#elif defined(SD_SCK) && defined(SOC_SDMMC_HOST_SUPPORTED)
#define FILESYSTEM SD_MMC
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */);
  if (!SD_MMC.begin(ROOT, true /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
#elif defined(SD_CS)
#define FILESYSTEM SD
  if (!SD.begin(SD_CS, SPI, 80000000, ROOT))
#else
// #define FILESYSTEM FFat
#define FILESYSTEM LittleFS
// #define FILESYSTEM SPIFFS
  if (!LittleFS.begin(false, ROOT))
#endif
  {
    Serial.println("ERROR: File system mount failed!");
    gfx->println("ERROR: File System Mount Failed!");
  }
  else
  {
    mjpeg_buf = (uint8_t *)aligned_alloc(16, MJPEG_BUFFER_SIZE);
    if (!mjpeg_buf)
    {
      Serial.println("mjpeg_buf malloc failed!");
    }
    else
    {
      output_buf = (uint16_t *)aligned_alloc(16, MJPEG_OUTPUT_SIZE);
      if (!output_buf)
      {
        Serial.println("output_buf malloc failed!");
      }
    }
  }
}

void loop()
{
  if (output_buf)
  {
    Serial.println("MJPEG start");

    start_ms = millis();
    curr_ms = millis();
    total_frames = 0;
    total_read_video = 0;
    total_decode_video = 0;
    total_show_video = 0;

#ifdef BIG_ENDIAN_PIXEL
    if (!mjpeg.setup(
            MJPEG_FILENAME, mjpeg_buf,
            output_buf, MJPEG_OUTPUT_SIZE, true /* useBigEndian */))
#else
    if (!mjpeg.setup(
            MJPEG_FILENAME, mjpeg_buf,
            output_buf, MJPEG_OUTPUT_SIZE, false /* useBigEndian */))
#endif
    {
      Serial.println("mjpeg.setup() failed!");
    }
    else
    {
      while (mjpeg.readMjpegBuf())
      {
        // Read video
        total_read_video += millis() - curr_ms;
        curr_ms = millis();

        // Play video
        mjpeg.decodeJpg();
        total_decode_video += millis() - curr_ms;
        curr_ms = millis();

        if (x == -1)
        {
          w = mjpeg.getWidth();
          h = mjpeg.getHeight();
          x = (w > gfx->width()) ? 0 : ((gfx->width() - w) / 2);
          y = (h > gfx->height()) ? 0 : ((gfx->height() - h) / 2);
        }
#if defined(CANVAS)
        g->draw16bitBeRGBBitmap(x, y, output_buf, w, h);
#elif defined(RGB_PANEL) || defined(DSI_PANEL)
        gfx->draw16bitBeRGBBitmap(x, y, output_buf, w, h);
        gfx->flush(true /* force_flush */);
#else
        gfx->draw16bitBeRGBBitmap(x, y, output_buf, w, h);
#endif
        total_show_video += millis() - curr_ms;

        curr_ms = millis();
        total_frames++;
      }
      mjpeg.close();

      int time_used = millis() - start_ms;
      Serial.println("MJPEG end");

      float fps = 1000.0 * total_frames / time_used;
      Serial.printf("Arduino_GFX ESP32 SIMD MJPEG decoder\n\n");
      Serial.printf("Frame size: %d x %d\n", mjpeg.getWidth(), mjpeg.getHeight());
      Serial.printf("Total frames: %d\n", total_frames);
      Serial.printf("Time used: %d ms\n", time_used);
      Serial.printf("Average FPS: %0.1f\n", fps);
      Serial.printf("Read MJPEG: %lu ms (%0.1f %%)\n", total_read_video, 100.0 * total_read_video / time_used);
      Serial.printf("Decode video: %lu ms (%0.1f %%)\n", total_decode_video, 100.0 * total_decode_video / time_used);
      Serial.printf("Show video: %lu ms (%0.1f %%)\n", total_show_video, 100.0 * total_show_video / time_used);
    }
  }
}
