# MiniTV

Various example for playing audio and video

## Convert audio + video for SD card

### AAC Audio

#### 44.1 kHz Mono
`ffmpeg -i input.mp4 -ar 44100 -ac 1 -ab 24k -filter:a loudnorm -filter:a "volume=-5dB" 44100.aac`

### MP3 Audio

#### 44.1 kHz Mono
`ffmpeg -i input.mp4 -ar 44100 -ac 1 -ab 32k -filter:a loudnorm -filter:a "volume=-5dB" 44100.mp3`

### MJPEG Video

#### 288x240@12fps
`ffmpeg -i input.mp4 -vf "fps=12,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 288_12fps.mjpeg`

#### 288x240@15fps
`ffmpeg -i input.mp4 -vf "fps=15,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 288_15fps.mjpeg`

#### 288x240@30fps
`ffmpeg -i input.mp4 -vf "fps=30,scale=-1:240:flags=lanczos,crop=288:in_h:(in_w-288)/2:0" -q:v 11 288_30fps.mjpeg`

#### 320x240@15fps
`ffmpeg -i input.mp4 -vf "fps=15,scale=-1:240:flags=lanczos,crop=320:in_h:(in_w-320)/2:0" -q:v 11 320_15fps.mjpeg`

#### 320x240@30fps
`ffmpeg -i input.mp4 -vf "fps=30,scale=-1:240:flags=lanczos,crop=320:in_h:(in_w-320)/2:0" -q:v 11 320_30fps.mjpeg`

#### 480x270@12fps
`ffmpeg -i input.mp4 -vf "fps=12,scale=480:-1:flags=lanczos" -q:v 9 480_12fps.mjpeg`

#### 480x272@15fps
`ffmpeg -i input.mp4 -vf "fps=15,scale=-1:272:flags=lanczos,crop=480:in_h:(in_w-480)/2:0" -q:v 9 480_15fps.mjpeg`

#### 480x272@30fps
`ffmpeg -i input.mp4 -vf "fps=30,scale=-1:272:flags=lanczos,crop=480:in_h:(in_w-480)/2:0" -q:v 9 480_30fps.mjpeg`

#### 800x@8fps
`ffmpeg -i input.mp4 -vf "fps=8,scale=-1:480:flags=lanczos,crop=800:in_h:(in_w-800)/2:0" -q:v 9 800_8fps.mjpeg`
