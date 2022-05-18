# MiniTV

Various example for playing audio and video

## Convert audio + video for SD card

### MP3 Audio

##### 22 kHz Mono
`ffmpeg -i input.mp4 -ar 22050 -ac 1 -ab 32 22050.mp3`

### MJPEG Video

#### 272x240@15fps

`ffmpeg -i input.mp4 -vf "fps=15,scale=-1:240:flags=lanczos,crop=272:in_h:(in_w-320)/2:0" -q:v 9 272_15fps.mjpeg`

#### 272x240@30fps

`ffmpeg -i input.mp4 -vf "fps=30,scale=-1:240:flags=lanczos,crop=272:in_h:(in_w-320)/2:0" -q:v 9 272_30fps.mjpeg`
