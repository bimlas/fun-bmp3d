# makefile.nix - *nix makefile

NAME   = bmp3d

CC     = gcc -ggdb
LIBS   = -lSDL -lSDL_ttf

all: bmp3d.c
	$(CC) bmp3d.c -o ../$(NAME).nix $(LIBS)
