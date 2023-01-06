/*
   require libraries:
   https://github.com/moononournation/Arduino_GFX.git
   https://github.com/pschatzmann/arduino-libhelix.git
   https://github.com/bitbank2/JPEGDEC.git
*/
const char *jpeg_file = "/SPY_WARS.jpg";
const char *mp3_files[] = {
    "/EP05.mp3",
    "/EP15.mp3",
    "/EP20.mp3"};
const char *mjpeg_files[] = {
    "/EP05.mjpeg",
    "/EP15.mjpeg",
    "/EP20.mjpeg"};
#define FPS 10
#define MJPEG_BUFFER_SIZE (288 * 240 * 2 / 10)
#define UP_BTN_PIN 0
#define DOWN_BTN_PIN 9

#include <WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include <SPIFFS.h>
#include <FFat.h>
#include <SD.h>
#include <SD_MMC.h>

/* Arduino_GFX */
#include <Arduino_GFX_Library.h>
#define GFX_BL 11
/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_ESP32SPI(6 /* DC */, 7 /* CS */, 2 /* SCK */, 3 /* MOSI */, GFX_NOT_DEFINED /* MISO */, FSPI /* spi_num */);
/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ST7789(bus, 10 /* RST */, 1 /* rotation */, true /* IPS */, 240 /* width */, 288 /* height */, 0 /* col offset 1 */, 20 /* row offset 1 */, 0 /* col offset 2 */, 12 /* row offset 2 */);

/* variables */
static int next_frame = 0;
static unsigned long start_ms, next_frame_ms;
static int8_t curr_video_idx = -1;
static bool btn_pressed = false;
static long last_pressed = -1;
static File aFile;
static File vFile;

/* MP3 audio */
#include "esp32_audio.h"

/* MJPEG Video */
#include "MjpegClass.h"
static MjpegClass mjpeg;
static uint8_t *mjpeg_buf;

// pixel drawing callback
static int drawMCU(JPEGDRAW *pDraw)
{
  // Serial.printf("Draw pos = (%d, %d), size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
} /* drawMCU() */

void IRAM_ATTR up_btn_pressed()
{
  if ((millis() - last_pressed) >= 500)
  {
    last_pressed = millis();

    curr_video_idx--;
    if (curr_video_idx < 0)
    {
      curr_video_idx = 2;
    }

    btn_pressed = true;
  }
}

void IRAM_ATTR down_btn_pressed()
{
  if ((millis() - last_pressed) >= 500)
  {
    last_pressed = millis();

    curr_video_idx++;
    if (curr_video_idx > 2)
    {
      curr_video_idx = 0;
    }

    btn_pressed = true;
  }
}

void setup()
{
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  // while (!Serial);

  // Init Display
  gfx->begin(80000000);
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  gfx->println("Init I2S");
  esp_err_t ret_val = i2s_init(I2S_NUM_0, 44100, -1 /* MCLK */, 4 /* SCLK */, 5 /* LRCK */, 8 /* DOUT */, -1 /* DIN */);
  if (ret_val != ESP_OK)
  {
    Serial.printf("i2s_init failed: %d\n", ret_val);
  }
  i2s_zero_dma_buffer(I2S_NUM_0);

  gfx->println("Init FS");
  // if (!LittleFS.begin(false, "/root"))
  // if (!SPIFFS.begin(false, "/root"))
  if (!FFat.begin(false, "/root"))

  // SPIClass spi = SPIClass(HSPI);
  // spi.begin(14 /* SCK */, 2 /* MISO */, 15 /* MOSI */, 13 /* CS */);
  // if (!SD.begin(13, spi, 80000000))

  // if ((!SD_MMC.begin("/root")) && (!SD_MMC.begin("/root")) && (!SD_MMC.begin("/root")) && (!SD_MMC.begin("/root"))) /* 4-bit SD bus mode */
  // if ((!SD_MMC.begin("/root", true)) && (!SD_MMC.begin("/root", true)) && (!SD_MMC.begin("/root", true)) && (!SD_MMC.begin("/root", true))) /* 1-bit SD bus mode */
  {
    Serial.println(F("ERROR: File system mount failed!"));
    gfx->println(F("ERROR: File system mount failed!"));
  }
  else
  {
    mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE);
    if (!mjpeg_buf)
    {
      Serial.println(F("mjpeg_buf malloc failed!"));
    }
    else
    {
      // File jFile = LittleFS.open(jpeg_file);
      // File jFile = SPIFFS.open(jpeg_file);
      File jFile = FFat.open(jpeg_file);
      // File jFile = SD.open(jpeg_file);
      // File jFile = SD_MMC.open(jpeg_file);
      if (!jFile || jFile.isDirectory())
      {
        Serial.printf("ERROR: Failed to open %s file for reading\n", jpeg_file);
        gfx->printf("ERROR: Failed to open %s file for reading\n", jpeg_file);
      }
      else
      {
        // abuse mjpeg class display JPEG image
        mjpeg.setup(
            &jFile, mjpeg_buf, drawMCU, true /* useBigEndian */,
            0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        mjpeg.readMjpegBuf();
        mjpeg.drawJpg();
        jFile.close();
      }
    }
  }

  /* Init buttons */
  pinMode(UP_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(UP_BTN_PIN, up_btn_pressed, FALLING);
  pinMode(DOWN_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(DOWN_BTN_PIN, down_btn_pressed, FALLING);
}

void loop()
{
  if (btn_pressed)
  {
    btn_pressed = false; // reset status

    // close previous files first
    if (vFile)
    {
      vFile.close();
    }
    if (aFile)
    {
      mp3_player_task_stop();
      aFile.close();
    }

    // aFile = LittleFS.open(mp3_files[curr_video_idx]);
    // aFile = SPIFFS.open(mp3_files[curr_video_idx]);
    aFile = FFat.open(mp3_files[curr_video_idx]);
    // aFile = SD.open(mp3_files[curr_video_idx]);
    // aFile = SD_MMC.open(mp3_files[curr_video_idx]);
    if (!aFile || aFile.isDirectory())
    {
      Serial.printf("ERROR: Failed to open %s file for reading\n", mp3_files[curr_video_idx]);
      gfx->printf("ERROR: Failed to open %s file for reading\n", mp3_files[curr_video_idx]);
    }
    else
    {
      Serial.printf("Open MP3 file %s.\n", mp3_files[curr_video_idx]);

      // vFile = LittleFS.open(mjpeg_files[curr_video_idx]);
      // vFile = SPIFFS.open(mjpeg_files[curr_video_idx]);
      vFile = FFat.open(mjpeg_files[curr_video_idx]);
      // vFile = SD.open(mjpeg_files[curr_video_idx]);
      // vFile = SD_MMC.open(mjpeg_files[curr_video_idx]);
      if (!vFile || vFile.isDirectory())
      {
        Serial.printf("ERROR: Failed to open %s file for reading\n", mjpeg_files[curr_video_idx]);
        gfx->printf("ERROR: Failed to open %s file for reading\n", mjpeg_files[curr_video_idx]);
      }
      else
      {
        Serial.printf("Open MJPEG file %s.\n", mjpeg_files[curr_video_idx]);

        // Start playing MP3
        BaseType_t ret_val = mp3_player_task_start(&aFile);
        if (ret_val != pdPASS)
        {
          Serial.printf("mp3_player_task_start failed: %d\n", ret_val);
        }

        // Start playing MJPEG
        mjpeg.setup(
            &vFile, mjpeg_buf, drawMCU, true /* useBigEndian */,
            0 /* x */, 0 /* y */, gfx->width() /* widthLimit */, gfx->height() /* heightLimit */);
        next_frame = 0;
        start_ms = millis();
        next_frame_ms = start_ms;
      }
    }
  }

  if (curr_video_idx < 0)
  {
    delay(1000); // do nothing at the beginning
  }
  else if (millis() < next_frame_ms)
  {
    vTaskDelay(pdMS_TO_TICKS(1)); // wait for next frame
  }
  else
  {
    next_frame_ms = start_ms + (++next_frame * 1000 / FPS);

    if (vFile.available() && mjpeg.readMjpegBuf()) // Read video
    {
      if (millis() < next_frame_ms) // check show frame or skip frame
      {
        // Play video
        mjpeg.drawJpg();
      }
      else
      {
        Serial.println(F("Skip frame"));
      }
    }
  }
}
