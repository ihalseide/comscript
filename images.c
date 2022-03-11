#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define COMSCRIPT_IMPLEMENTATION
#include "comscript.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

struct Color
{
	int r;
	int g;
	int b;
};

struct Image
{
	int width;
	int height;
	struct Color *colors;
};

static struct Image **images = NULL;
void img_alloc(void)
{

	int height = dpop();
	int width = dpop();

	struct Image *new = malloc(sizeof(*new));
	new->width = width;
	new->height = height;
	new->colors = malloc(sizeof(*new->colors));

	int imageIndex = arrlen(images);
	arrpush(images, new);
	dpush(imageIndex);
}
void img_free(void)
{
	int imageIndex = dpop();
	if (0 <= imageIndex && imageIndex < arrlen(images))
	{
		struct Image *p = images[imageIndex];
		free(p->colors);
		free(p);
	}
}

struct WordLookup words[] =
{
	{3, "bye",   bye,       0, 0},
	{3, "out",   out,       1, 0},
	{5, "alloc", img_alloc, 2, 1},
	{4, "free",  img_free,  1, 0},
};
struct WordDict dict =
{
	.len = sizeof(words)/sizeof(words[0]),
	.words = words,
};

int main(void)
{
	const char s[] = "16 16 alloc dup . free";
	runScript(strlen(s), s, &dict);
	return 0;
}

