#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DATA_STACK_SZ 128
#define COMSCRIPT_IMPLEMENTATION
#include "comscript.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

struct Image
{
	int width;
	int height;
	int *colors;
};

// Array for holding allocated images
struct Image **imagesArr = NULL;

int is_img(int i)
{
	return 0 <= i && i < arrlen(imagesArr);
}

// ( r g b a -- rgba )
void rgba(void)
{
	char a = dpop();
	char b = dpop();
	char g = dpop();
	char r = dpop();
	dpush( (r << 24) + (g << 16) + (b << 8) + a );
}

// ( r g b -- rgba )
void rgb(void)
{
	char b = dpop();
	char g = dpop();
	char r = dpop();
	dpush( (r << 24) + (g << 16) + (b << 8) + 255 );
}

// ( v -- rgba )
void value(void)
{
	char v = dpop();
	dpush( (v << 24) + (v << 16) + (v << 8) + 255 );
}

void img_width(void)
{
	int img = dtop();
	if (is_img(img))
	{
		struct Image *p = imagesArr[img];
		dpush(p->width);
	}
	else
	{
		dpush(-1);
	}
}

void img_height(void)
{
	int img = dtop();
	if (is_img(img))
	{
		struct Image *p = imagesArr[img];
		dpush(p->height);
	}
	else
	{
		dpush(-1);
	}
}

// ( img x0 y0 w h val -- img ) draw rectangle
void img_rect(void)
{
	int val = dpop();
	int h = dpop();
	int w = dpop();
	int y0 = dpop();
	int x0 = dpop();
	int img = dtop();

	if (!is_img(img))
	{
		printf("rect: invalid image\n");
		return;
	}
	struct Image *p = imagesArr[img];
	int imgW = p->width;
	int imgH = p->height;

	// Horizontal lines
	for (int x = x0; x < (x0 + w) && x < imgW; x++)
	{
		int i;
		i = x + y0 * imgW;
		p->colors[i] = val;
		i = x + (y0 + h) * imgW;
		p->colors[i] = val;
	}

	// Vertical lines
	for (int y = y0; y < (y0 + h) && y < imgH; y++)
	{
		int i;
		i = x0 + y * imgW;
		p->colors[i] = val;
		i = (x0 + w) + y * imgW;
		p->colors[i] = val;
	}
}

// ( img x0 y0 w h val -- img ) fill rectangle
void img_fillrect(void)
{
	int val = dpop();
	int h = dpop();
	int w = dpop();
	int y0 = dpop();
	int x0 = dpop();
	int img = dtop();

	if (!is_img(img))
	{
		printf("rect: invalid image\n");
		return;
	}
	struct Image *p = imagesArr[img];
	int imgW = p->width;
	int imgH = p->height;

	for (int x = x0; x < (x0 + w) && x < imgW; x++)
	{
		for (int y = y0; y < (y0 + h) && y < imgH; y++)
		{
			int i = x + y * imgW;
			p->colors[i] = val;
		}
	}
}

// ( img x0 y0 x1 y1 val -- img ) draw line
void img_line(void)
{

}

void img_alloc(void)
{

	int height = dpop();
	int width = dpop();
	int size = width * height;

	struct Image *new = malloc(sizeof(*new));
	new->width = width;
	new->height = height;
	new->colors = malloc(size * sizeof(*new->colors));

	// Add it to the allocated array
	// Look for holes
	for (int i = 0; i < arrlen(imagesArr); i++)
	{
		if (imagesArr[i] == NULL)
		{
			imagesArr[i] = new;
			dpush(i);
			return;
		}
	}
	// No holes
	int i = arrlen(imagesArr);
	arrpush(imagesArr, new);
	dpush(i);
}
void img_free(void)
{
	int imageIndex = dpop();
	if (0 <= imageIndex && imageIndex < arrlen(imagesArr))
	{
		struct Image *p = imagesArr[imageIndex];

		free(p->colors);
		free(p);
		imagesArr[imageIndex] = NULL; // put a 'hole' in the array
	}
}

// Clear image to certain value
// ( img v -- img )
void img_clear(void)
{
	int val = dpop();
	int img = dpick(0);
	if (!is_img(img))
	{
		// invalid image index
		printf("clear: invalid image\n");
		return;
	}
	else
	{
		assert(imagesArr);
		struct Image *p = imagesArr[img];
		assert(p);
		int size = p->width * p->height;
		for (int i = 0; i < size; i++)
		{
			p->colors[i] = val;
		}
	}
}

// ( img x y -- img val )
void img_get(void)
{
	int y = dpop();
	int x = dpop();
	int img = dtop();
	if (is_img(img))
	{
		// invalid image index
		dpush(0);
	}
	else
	{
		struct Image *p = imagesArr[img];
		int j = x + y * p->width;
		dpush(p->colors[j]);
	}
}

// ( img x y val -- img val )
void img_set(void)
{
	int val = dpop();
	int y = dpop();
	int x = dpop();
	int img = dpick(0);
	dpush(val);

	if (is_img(img))
	{
		struct Image *p = imagesArr[img];
		int i = x + y * p->width;
		int size = p->width * p->height;
		if (0 <= i && i < size)
		{
			p->colors[i] = val;
		}
	}
}

// ( img x1 y1 x2 y2 -- img ) swaps values in image
void img_swap(void)
{
	int y2 = dpop();
	int x2 = dpop();
	int y1 = dpop();
	int x1 = dpop();
	int img = dtop();

	if (is_img(img))
	{
		struct Image *p = imagesArr[img];
		int i1 = x1 + y1 * p->width;
		int i2 = x2 + y2 * p->width;
		int size = p->width * p->height;
		if (i1 != i2 && 0 <= i1 && i1 < size && 0 <= i2 && i2 < size)
		{
			int temp = p->colors[i1];
			p->colors[i1] = p->colors[i2];
			p->colors[i2] = temp;
		}
	}
}

// ( img1 -- img1 img2 ) makes a copy of an image
void img_copy(void)
{
	int img1 = dtop();
	if (is_img(img1))
	{
		struct Image *p1 = imagesArr[img1];

		// Allocate a new image
		dpush(p1->width);
		dpush(p1->height);
		img_alloc();

		int img2 = dtop();
		struct Image *p2 = imagesArr[img2];

		assert(p1 != p2);
		assert(p1->width == p2->width);
		assert(p1->height == p2->height);

		int size = p1->width * p1->width;
		for (int i = 0; i < size; i++)
		{
			p2->colors[i] = p1->colors[i];
		}
	}
	else
	{
		dpush(-1);
	}
}

// Display an image
void img_disp(void)
{
	int img = dtop();
	if (is_img(img))
	{
		assert(imagesArr);
		struct Image *p = imagesArr[img];
		assert(p);
		printf("Image #%d (%dx%d):\n", img, p->width, p->height);
		for (int x = 0; x < p->width; x++)
		{
			for (int y = 0; y < p->height; y++)
			{
				int j = x + y * p->width;
				printf(" %2d", p->colors[j]);
			}
			putchar('\n');
		}
	}
}

void dprint(void)
{
	assert(dI >= 1);
	printf("%d", dpop());
}

void space(void)
{
	putchar(' ');
}

void line(void)
{
	putchar('\n');
}

void emit(void)
{
	putchar(dpop());
}

void dispstack(void)
{
	printf("stack:");
	if (dI > 0)
	{
		for (int i = dI - 1; i >= 0; i--)
		{
			printf(" %d", dstack[i]);
		}
	}
	else
	{
		printf(" empty");
	}
	putchar('\n');
}

// ( img name -- img )
void img_save(void)
{
	int suffix = dpop();
	int img = dtop();

	if (!is_img(img))
	{
		return;
	}
	struct Image *p = imagesArr[img];

	char name[100];
	snprintf(name, sizeof(name), "img%d.png", suffix);

	int width = p->width;
	int height = p->height;

	void *data = p->colors;
	int comp = sizeof(*p->colors);
	int stride = width * sizeof(*p->colors);
	int success = stbi_write_png(name, width, height, comp, data, stride);

	if (!success)
	{
		printf("could not save image #%d\n", img);
	}
}

struct WordLookup words[] =
{
	{ 1, "+",     add,       2, 1 },
	{ 1, "-",     subtract,  2, 1 },
	{ 1, "*",     multiply,  2, 1 },
	{ 1, "/",     divide,    2, 1 },
	{ 4, "drop",  drop,      1, 0 },
	{ 3, "dup",   duplicate, 1, 2 },
	{ 4, "swap",  swap,      2, 2 },
	{ 4, "over",  over,      2, 3 },
	{ 3, "nip",   nip,       2, 1 },

	{ 2, ".s",    dispstack, 0, 0 },
	{ 1, ".",     dprint,    1, 0 },
	{ 5, "space", space,     0, 0 },
	{ 4, "line",  line,      0, 0 },
	{ 4, "emit",  emit,      1, 0 },

	{ 3, "rgb",   rgb,       3, 1 }, // ( r g b -- rgba )
	{ 4, "rgba",  rgba,      4, 1 }, // ( r g b a -- rgba )
	{ 5, "value", value,     1, 1 }, // ( val -- rgba )

	{ 7, "display", img_disp,   1, 1 }, // ( img -- img )
	{ 5, "alloc",   img_alloc,  2, 1 }, // ( w h -- img )
	{ 4, "free",    img_free,   1, 0 }, // ( img -- )
	{ 5, "width",   img_width,  1, 2 }, // ( img -- img w )
	{ 6, "height",  img_height, 1, 2 }, // ( img -- img h )
	{ 5, "clear",   img_clear,  2, 1 }, // ( img val -- img )
	{ 3, "get",     img_get,    3, 2 }, // ( img x y -- img val )
	{ 3, "set",     img_set,    4, 2 }, // ( img x y val -- img val )
	{ 5, "iswap",   img_swap,   5, 1 }, // ( img x1 y1 x2 y2 -- img ) swaps values in image
	{ 4, "copy",    img_copy,   1, 2 }, // ( img1 -- img1 img2 ) makes a copy of an image
	{ 4, "save",    img_save,   2, 1 }, // ( img name -- img ) name is a number to append to filename
	{ 4, "rect",     img_rect,     5, 1 }, // ( img x0 y0 w h val -- img ) draw rectangle
	{ 8, "fillrect", img_fillrect, 5, 1 }, // ( img x0 y0 w h val -- img ) fill rectangle
	{ 4, "line",     img_line,     5, 1 }, // ( img x0 y0 x1 y1 val -- img ) draw line
};
struct WordDict dict =
{
	.len = sizeof(words)/sizeof(words[0]),
	.words = words,
};

int main(void)
{
	const char s[] = "64 dup alloc 255 255 255 rgb clear 5 5 10 10 255 value fillrect 55 save free";
	int code = runScript(strlen(s), s, &dict);
	if (code)
	{
		printf("error: %d: %s\n", code, errMessage(code));
	}
	else
	{
		printf("success\n");
	}
	return code;
}

