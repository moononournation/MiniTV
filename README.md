# MiniTV

Various example for playing audio and video

## Convert audio + video for SD card

### AAC Audio

##### 22 kHz Mono
`ffmpeg -i input.mp4 -ar 22050 -ac 1 -ab 24k 22050.aac`

### MP3 Audio

##### 22 kHz Mono
`ffmpeg -i input.mp4 -ar 22050 -ac 1 -ab 32k 22050.mp3`

### MJPEG Video

#### 288x240@15fps

`ffmpeg -i input.mp4 -vf "fps=15,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 288_15fps.mjpeg`

#### 288x240@30fps

`ffmpeg -i input.mp4 -vf "fps=30,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 288_30fps.mjpeg`

#### 320x240@15fps

`ffmpeg -i input.mp4 -vf "fps=15,scale=-1:240:flags=lanczos,crop=320:in_h:(in_w-320)/2:0" -q:v 11 320_15fps.mjpeg`

#### 320x240@30fps

`ffmpeg -i input.mp4 -vf "fps=30,scale=-1:240:flags=lanczos,crop=320:in_h:(in_w-320)/2:0" -q:v 11 320_30fps.mjpeg`
