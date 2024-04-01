# MiniTV

Various example for playing audio and video

## Convert audio + video for SD card

### AAC Audio

#### 44.1 kHz Mono

```console
ffmpeg -i input.mp4 -ar 44100 -ac 1 -ab 24k -filter:a loudnorm -filter:a "volume=-5dB" 44100.aac
```

### MP3 Audio

#### 44.1 kHz Mono

```console
ffmpeg -i input.mp4 -ar 44100 -ac 1 -ab 32k -filter:a loudnorm -filter:a "volume=-5dB" 44100.mp3
```

### MJPEG Video

#### 288x240@12fps

```console
ffmpeg -i input.mp4 -vf "fps=12,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 288_12fps.mjpeg
```

#### 288x240@15fps

```console
ffmpeg -i input.mp4 -vf "fps=15,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 288_15fps.mjpeg
```

#### 288x240@30fps

```console
ffmpeg -i input.mp4 -vf "fps=30,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 288_30fps.mjpeg
```

#### 320x240@15fps

```console
ffmpeg -i input.mp4 -vf "fps=15,scale=-1:240:flags=lanczos,crop=320:in_h:(in_w-320)/2:0" -q:v 11 320_15fps.mjpeg
```

#### 320x240@30fps

```console
ffmpeg -i input.mp4 -vf "fps=30,scale=-1:240:flags=lanczos,crop=320:in_h:(in_w-320)/2:0" -q:v 11 320_30fps.mjpeg
```

#### 480x270@12fps

```console
ffmpeg -i input.mp4 -vf "fps=12,scale=480:-1:flags=lanczos" -q:v 9 480_12fps.mjpeg
```

#### 480x272@15fps

```console
ffmpeg -i input.mp4 -vf "fps=15,scale=-1:272:flags=lanczos,crop=480:in_h:(in_w-480)/2:0" -q:v 9 480_15fps.mjpeg
```

#### 480x272@30fps

```console
ffmpeg -i input.mp4 -vf "fps=30,scale=-1:272:flags=lanczos,crop=480:in_h:(in_w-480)/2:0" -q:v 9 480_30fps.mjpeg
```

#### 800x@8fps

```console
ffmpeg -i input.mp4 -vf "fps=8,scale=-1:480:flags=lanczos,crop=800:in_h:(in_w-800)/2:0" -q:v 9 800_8fps.mjpeg
```

## SPY×FAMILY

### EP05

```console
ffmpeg -i spy_family/EP05.mp4 -ss 07:25.70 -t 00:31.90 -vf "scale=320:264:flags=lanczos,crop=288:240:(in_w-288)/2:0" EP05.mpg
ffmpeg -i spy_family/EP05.mp4 -ss 07:25.70 -t 00:32.00 -ar 44100 -ac 1 -ab 16k -filter:a loudnorm -filter:a "volume=-1dB" EP05.mp3
ffmpeg -i spy_family/EP05.mp4 -ss 07:25.70 -t 00:31.90 -vf "fps=10,scale=320:264:flags=lanczos,crop=288:240:(in_w-288)/2:0" -q:v 12 EP05.mjpeg
```

### EP15

```console
ffmpeg -i spy_family/EP15.mp4 -ss 20:00.00 -t 00:12.00 -vf "scale=320:264:flags=lanczos,crop=288:240:(in_w-288)/2:0" EP15.mpg
ffmpeg -i spy_family/EP15.mp4 -ss 20:00.00 -t 00:12.00 -ar 44100 -ac 1 -ab 16k -filter:a loudnorm -filter:a "volume=-5dB" EP15.mp3
ffmpeg -i spy_family/EP15.mp4 -ss 20:00.00 -t 00:12.00 -vf "fps=10,scale=320:264:flags=lanczos,crop=288:240:(in_w-288)/2:0" -q:v 12 EP15.mjpeg
```

### EP20

```console
ffmpeg -i spy_family/EP20.mp4 -ss 17:02.00 -t 00:14.00 -vf "scale=320:264:flags=lanczos,crop=288:240:(in_w-288)/2:0" EP20.mpg
ffmpeg -i spy_family/EP20.mp4 -ss 17:02.00 -t 00:14.00 -ar 44100 -ac 1 -ab 16k -filter:a loudnorm -filter:a "volume=-5dB" EP20.mp3
ffmpeg -i spy_family/EP20.mp4 -ss 17:02.00 -t 00:14.00 -vf "fps=10,scale=320:264:flags=lanczos,crop=288:240:(in_w-288)/2:0" -q:v 12 EP20.mjpeg
```
