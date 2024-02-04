# makefile.nix - *nix makefile

NAME   = bmp3d

CC     = gcc
LIBS   = -lSDL -lSDL_ttf -lm

all: bmp3d.c
	$(CC) bmp3d.c -o $(NAME) $(LIBS)
