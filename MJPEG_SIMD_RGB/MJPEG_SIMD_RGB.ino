/***
 * Required libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * ESP32_JPEG: https://github.com/esp-arduino-libs/ESP32_JPEG.git
 */

#define MJPEG_FILENAME "/earth480.mjpeg"
#define MJPEG_BUFFER_SIZE (480 * 480 * 2 / 10)
#define MJPEG_OUTPUT_SIZE (480 * 480 * 2)

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE MAKERFABS_ESP32_S3_TFT_4_0
Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC */, 1 /* CS */,
    12 /* SCK */, 11 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    45 /* DE */, 4 /* VSYNC */, 5 /* HSYNC */, 21 /* PCLK */,
    39 /* R0 */, 40 /* R1 */, 41 /* R2 */, 42 /* R3 */, 2 /* R4 */,
    0 /* G0 */, 9 /* G1 */, 14 /* G2 */, 47 /* G3 */, 48 /* G4 */, 3 /* G5 */,
    6 /* B0 */, 7 /* B1 */, 15 /* B2 */, 16 /* B3 */, 8 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

#include <FFat.h>

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
        // uint16_t *output_buf = (uint16_t *)malloc(MJPEG_OUTPUT_SIZE);

        Serial.println(F("MJPEG start"));

        start_ms = millis();
        curr_ms = millis();
        mjpeg.setup(
            &mjpegFile, mjpeg_buf, gfx->getFramebuffer(), true /* useBigEndian */,
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
          gfx->flush();
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
        gfx->printf("ESP32 JPEG SIMD Decoder\n\n");
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
