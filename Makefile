default: debug

debug: images.c comscript.h
	cc -Wall -g images.c -o images -lm

normal: images.c comscript.h
	cc -Wall images.c -o images -lm
