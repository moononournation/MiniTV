/*******************************************************************************
 * MJPEG function
 *
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 ******************************************************************************/
#pragma once

#include <LittleFS.h>
#include <SD.h>
#ifdef SOC_SDMMC_HOST_SUPPORTED
#include <SD_MMC.h>
#endif

#include <JPEGDEC.h>
JPEGDEC jpeg;

// Functions to access a file
bool isFileOpened = false;
bool isJpegEnded = false;
bool isMjpegEnded = false;
File mjpegFile;
int32_t mjpeg_offset = 0;

void *mjpegOpenLittleFS(const char *filename, int32_t *size) {
  if ((!isFileOpened) || (!mjpegFile)) {
    mjpegFile = LittleFS.open(filename);
    if (mjpegFile) {
      mjpeg_offset = 0;
      isFileOpened = true;
      isMjpegEnded = false;
    }
  }
  isJpegEnded = false;
  *size = mjpegFile.size() - mjpeg_offset;
  return &mjpegFile;
}

void *mjpegOpenSD(const char *filename, int32_t *size) {
  if ((!isFileOpened) || (!mjpegFile)) {
    mjpegFile = SD.open(filename);
    if (mjpegFile) {
      mjpeg_offset = 0;
      isFileOpened = true;
      isMjpegEnded = false;
    }
  }
  isJpegEnded = false;
  *size = mjpegFile.size() - mjpeg_offset;
  return &mjpegFile;
}

#ifdef SOC_SDMMC_HOST_SUPPORTED
void *mjpegOpenSD_MMC(const char *filename, int32_t *size) {
  if ((!isFileOpened) || (!mjpegFile)) {
    mjpegFile = SD_MMC.open(filename);
    if (mjpegFile) {
      mjpeg_offset = 0;
      isFileOpened = true;
      isMjpegEnded = false;
    }
  }
  isJpegEnded = false;
  *size = mjpegFile.size() - mjpeg_offset;
  return &mjpegFile;
}
#endif

void mjpegClose(void *handle) {
  if (isFileOpened) {
    if (mjpegFile.available()) {
      mjpeg_offset = mjpegFile.position();
    } else {
      Serial.printf("mjpegClose() last mjpeg_offset: %d\n", mjpeg_offset);
      mjpegFile.close();
      isFileOpened = false;
      isMjpegEnded = true;
    }
  }
}

void mjpegCloseLoopback(void *handle) {
  // Serial.printf("mjpegCloseLoopback() last mjpeg_offset: %d\n", mjpeg_offset);
  if (isFileOpened) {
    if (mjpegFile.available()) {
      mjpeg_offset = mjpegFile.position();
    } else {
      Serial.printf("mjpegCloseLoopback() last mjpeg_offset: %d\n", mjpeg_offset);
      mjpegFile.seek(0);
      mjpeg_offset = 0;
    }
  }
}

int32_t mjpegRead(JPEGFILE *handle, uint8_t *buffer, int32_t length) {
  if ((!isFileOpened) || (!mjpegFile)) {
    return 0;
  }
  if ((!isJpegEnded) && mjpegFile.available()) {
    // Serial.printf("%04X: read %02X\n", mjpegFile.position(), length);
    int32_t r = 0;
    uint8_t last_b = 0;
    while (length--) {
      uint8_t b = mjpegFile.read();
      buffer[r++] = b;
      if ((last_b == 0xFF) && (b == 0xD9)) {
        isJpegEnded = true;
        return r;
      }
      last_b = b;
    }
    return r;
  } else {
    return 0;
  }
}

int32_t mjpegSeek(JPEGFILE *handle, int32_t position) {
  Serial.printf("SEEK: %04X: , mjpeg_offset: %04X\n", position, mjpeg_offset);
  if ((!isFileOpened) || (!mjpegFile)) {
    return 0;
  }
  return mjpegFile.seek(mjpeg_offset + position);
}
