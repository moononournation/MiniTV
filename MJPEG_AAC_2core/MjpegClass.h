/*******************************************************************************
 * JPEGDEC Wrapper Class
 *
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 ******************************************************************************/
#ifndef _MJPEGCLASS_H_
#define _MJPEGCLASS_H_

#define READ_BUFFER_SIZE 1024
// #define MAXOUTPUTSIZE (MAX_BUFFERED_PIXELS / 16 / 16)
#define MAXOUTPUTSIZE (288 / 3 / 16)
// #define NUMBER_OF_DRAW_BUFFER 4
#define NUMBER_OF_DRAW_BUFFER 6

#include <FS.h>

#include <JPEGDEC.h>

typedef struct
{
  xQueueHandle xqh;
  JPEG_DRAW_CALLBACK *drawFunc;
} paramDrawTask;

static JPEGDRAW jpegdraws[NUMBER_OF_DRAW_BUFFER];
static int _draw_queue_cnt = 0;
static int _draw_cnt = 0;
static xQueueHandle _xqh;

static int queueDrawMCU(JPEGDRAW *pDraw)
{
  int len = pDraw->iWidth * pDraw->iHeight * 2;
  JPEGDRAW *j = &jpegdraws[_draw_queue_cnt % NUMBER_OF_DRAW_BUFFER];
  j->x = pDraw->x;
  j->y = pDraw->y;
  j->iWidth = pDraw->iWidth;
  j->iHeight = pDraw->iHeight;
  memcpy(j->pPixels, pDraw->pPixels, len);

  // log_i("queueDrawMCU start.");
  ++_draw_queue_cnt;
  if ((_draw_queue_cnt - _draw_cnt) > NUMBER_OF_DRAW_BUFFER)
  {
    log_i("draw queue overflow!");
    while ((_draw_queue_cnt - _draw_cnt) > NUMBER_OF_DRAW_BUFFER)
    {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }

  xQueueSend(_xqh, &j, portMAX_DELAY);
  // log_i("queueDrawMCU end.\n");

  return 1;
}

static void draw_task(void *arg)
{
  paramDrawTask *p = (paramDrawTask *)arg;
  JPEGDRAW *pDraw;
  log_i("draw_task start.\n");
  while (xQueueReceive(p->xqh, &pDraw, portMAX_DELAY))
  {
    // log_i("draw_task work start: x: %d, y: %d, iWidth: %d, iHeight: %d.\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
    p->drawFunc(pDraw);
    // log_i("draw_task work end.\n");
    ++_draw_cnt;
  }
  vQueueDelete(p->xqh);
  log_i("draw_task end.\n");
  vTaskDelete(NULL);
}

class MjpegClass
{
public:
  bool setup(
      Stream *input, uint8_t *mjpeg_buf, JPEG_DRAW_CALLBACK *pfnDraw, bool useBigEndian,
      int x, int y, int widthLimit, int heightLimit)
  {
    _input = input;
    _mjpeg_buf = mjpeg_buf;
    _useBigEndian = useBigEndian;
    _x = x;
    _y = y;
    _widthLimit = widthLimit;
    _heightLimit = heightLimit;
    _inputindex = 0;

    _read_buf = (uint8_t *)malloc(READ_BUFFER_SIZE);

    _xqh = xQueueCreate(NUMBER_OF_DRAW_BUFFER, sizeof(JPEGDRAW));
    _pDrawTask.xqh = _xqh;
    _pDrawTask.drawFunc = pfnDraw;
    xTaskCreatePinnedToCore(
        (TaskFunction_t)draw_task,
        (const char *const)"Draw Task",
        (const uint32_t)2000,
        (void *const)&_pDrawTask,
        (UBaseType_t)configMAX_PRIORITIES - 1,
        (TaskHandle_t *const)&_drawTask,
        (const BaseType_t)0);

    for (int i = 0; i < NUMBER_OF_DRAW_BUFFER; i++)
    {
      if (!jpegdraws[i].pPixels)
      {
        jpegdraws[i].pPixels = (uint16_t *)heap_caps_malloc(MAXOUTPUTSIZE * 16 * 16 * 2, MALLOC_CAP_DMA);
      }
      if (jpegdraws[i].pPixels)
      {
        log_i("#%d draw buffer allocated.\n", i);
      }
    }

    return true;
  }

  bool readMjpegBuf()
  {
    if (_inputindex == 0)
    {
      _buf_read = _input->readBytes(_read_buf, READ_BUFFER_SIZE);
      _inputindex += _buf_read;
    }
    _mjpeg_buf_offset = 0;
    int i = 0;
    bool found_FFD8 = false;
    while ((_buf_read > 0) && (!found_FFD8))
    {
      i = 0;
      while ((i < _buf_read) && (!found_FFD8))
      {
        if ((_read_buf[i] == 0xFF) && (_read_buf[i + 1] == 0xD8)) // JPEG header
        {
          // log_i("Found FFD8 at: %d.\n", i);
          found_FFD8 = true;
        }
        ++i;
      }
      if (found_FFD8)
      {
        --i;
      }
      else
      {
        _buf_read = _input->readBytes(_read_buf, READ_BUFFER_SIZE);
      }
    }
    uint8_t *_p = _read_buf + i;
    _buf_read -= i;
    bool found_FFD9 = false;
    if (_buf_read > 0)
    {
      i = 3;
      while ((_buf_read > 0) && (!found_FFD9))
      {
        if ((_mjpeg_buf_offset > 0) && (_mjpeg_buf[_mjpeg_buf_offset - 1] == 0xFF) && (_p[0] == 0xD9)) // JPEG trailer
        {
          // log_i("Found FFD9 at: %d.\n", i);
          found_FFD9 = true;
        }
        else
        {
          while ((i < _buf_read) && (!found_FFD9))
          {
            if ((_p[i] == 0xFF) && (_p[i + 1] == 0xD9)) // JPEG trailer
            {
              found_FFD9 = true;
              ++i;
            }
            ++i;
          }
        }

        // log_i("i: %d\n", i);
        memcpy(_mjpeg_buf + _mjpeg_buf_offset, _p, i);
        _mjpeg_buf_offset += i;
        size_t o = _buf_read - i;
        if (o > 0)
        {
          // log_i("o: %d\n", o);
          memcpy(_read_buf, _p + i, o);
          _buf_read = _input->readBytes(_read_buf + o, READ_BUFFER_SIZE - o);
          _p = _read_buf;
          _inputindex += _buf_read;
          _buf_read += o;
          // log_i("_buf_read: %d\n", _buf_read);
        }
        else
        {
          _buf_read = _input->readBytes(_read_buf, READ_BUFFER_SIZE);
          _p = _read_buf;
          _inputindex += _buf_read;
        }
        i = 0;
      }
      if (found_FFD9)
      {
        return true;
      }
    }

    return false;
  }

  bool drawJpg()
  {
    _remain = _mjpeg_buf_offset;
    _jpeg.openRAM(_mjpeg_buf, _remain, queueDrawMCU);
    if (_scale == -1)
    {
      // scale to fit height
      int w = _jpeg.getWidth();
      int h = _jpeg.getHeight();
      float ratio = (float)h / _heightLimit;
      if (ratio <= 1)
      {
        _scale = 0;
      }
      else if (ratio <= 2)
      {
        _scale = JPEG_SCALE_HALF;
        w /= 2;
        h /= 2;
      }
      else if (ratio <= 4)
      {
        _scale = JPEG_SCALE_QUARTER;
        w /= 4;
        h /= 4;
      }
      else
      {
        _scale = JPEG_SCALE_EIGHTH;
        w /= 8;
        h /= 8;
      }
      _x = (w > _widthLimit) ? 0 : ((_widthLimit - w) / 2);
      _y = (_heightLimit - h) / 2;
    }
    _jpeg.setMaxOutputSize(MAXOUTPUTSIZE);
    if (_useBigEndian)
    {
      _jpeg.setPixelType(RGB565_BIG_ENDIAN);
    }
    // log_i("_jpeg.decode(%d, %d, _scale);", _x, _y);
    _jpeg.decode(_x, _y, _scale);
    _jpeg.close();

    return true;
  }

private:
  Stream *_input;
  uint8_t *_mjpeg_buf;
  bool _useBigEndian;
  int _x;
  int _y;
  int _widthLimit;
  int _heightLimit;

  uint8_t *_read_buf;
  int32_t _mjpeg_buf_offset = 0;

  TaskHandle_t _drawTask;
  paramDrawTask _pDrawTask;

  JPEGDEC _jpeg;
  int _scale = -1;

  int32_t _inputindex = 0;
  int32_t _buf_read;
  int32_t _remain = 0;
};

#endif // _MJPEGCLASS_H_
