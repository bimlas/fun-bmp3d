# Bmp3D

A fun project which turns bitmap image into relief.

I did this project a long time ago and didn't thought that I will push it to
the cloud, thus I wrote it in Hungarian.

```
$ apt install libsdl1.2-dev libsdl-ttf2.0-dev
$ make
$ ./bmp3d map.bmp
```

The BMP image has to be a 256 color BMP image.

Move by mouse + (NumPad or WASD):

```
       +    -
x      6 D  4 A
y      8 W  2 S
z      7 R  1 F
radius + T  - G

Change the target of projection (planar / sphere): 5, or SPACE
Turn on minimap:                                   Enter
Rapid movement:                                    Caps Lock
```
