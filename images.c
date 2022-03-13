#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DATA_STACK_SZ 128
#define COMSCRIPT_IMPLEMENTATION
#include "comscript.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

struct CodeQuote
{
	int length;
	const char *start;
};

struct WordDict dict;

// Array for holding allocated images
struct Image **imagesArr = NULL;

// Array for holding allocated code quotations
struct CodeQuote **quotesArr = NULL;

// Add an image pointer to the imagesArr and return image ID/index.
int ImageAdd(struct Image *p)
{
	// Look for NULL spots in the array
	for (int i = 0; i < arrlen(imagesArr); i++)
	{
		if (imagesArr[i] == NULL)
		{
			imagesArr[i] = p;
			dpush(i);
			return i;
		}
	}

	// No holes found, or imagesArr is empty
	int i = arrlen(imagesArr);
	arrpush(imagesArr, p);
	return i;
}

int is_img(int i)
{
	return 0 <= i && i < arrlen(imagesArr);
}

int is_quote(int i)
{
	return 0 <= i && i < arrlen(quotesArr);
}

// ( r g b a -- rgba )
void rgba(void)
{
	unsigned char a = dpop();
	unsigned char b = dpop();
	unsigned char g = dpop();
	unsigned char r = dpop();
	int rgba = r + (g << 8) + (b << 16) + (a << 24);
	dpush(rgba);
}

// ( r g b -- rgba )
void rgb(void)
{
	unsigned char b = dpop();
	unsigned char g = dpop();
	unsigned char r = dpop();
	unsigned int rgba = r + (g << 8) + (b << 16) + (255 << 24);
	dpush(rgba);
}

// ( v -- rgba )
void value(void)
{
	unsigned char v = dpop();
	int rgba = v + (v << 8) + (v << 16) + (255 << 24);
	dpush(rgba);
}

// ( rgba -- rgba )
void rgb_invert(void)
{
	unsigned int rgba = dpop();
	unsigned char r = ~(rgba & 255);
	unsigned char g = ~((rgba >> 8) & 255);
	unsigned char b = ~((rgba >> 16) & 255);
	unsigned char a = (rgba >> 24) & 255;
	unsigned int rgba2 = r + (g << 8) + (b << 16) + (a << 24);
	dpush(rgba2);
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

// ( img x0 y0 w h rgba -- img ) draw rectangle
void img_rect(void)
{
	unsigned int val = dpop();
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

	if (x0 + w > imgW)
	{
		printf("rect: too wide for image");
		return;
	}

	if (y0 + h > imgH)
	{
		printf("rect: too high for image");
		return;
	}

	// Horizontal lines
	for (int x = x0; x < (x0 + w) && x < imgW; x++)
	{
		int i;
		i = x + y0 * imgW;
		p->colors[i] = val;
		i = x + (y0 + h - 1) * imgW;
		p->colors[i] = val;
	}

	// Vertical lines
	for (int y = y0; y < (y0 + h) && y < imgH; y++)
	{
		int i;
		i = x0 + y * imgW;
		p->colors[i] = val;
		i = (x0 + w - 1) + y * imgW;
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

	if (x0 + w > imgW)
	{
		printf("rect: too wide for image");
		return;
	}

	if (y0 + h > imgH)
	{
		printf("rect: too high for image");
		return;
	}

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
	int val = dpop();
	int y1 = dpop();
	int x1 = dpop();
	int y0 = dpop();
	int x0 = dpop();
	int img = dtop();

	if (!is_img(img))
	{
		printf("error: line: not an image\n");
		return;
	}
	struct Image *p = imagesArr[img];

	if (x0 < 0 || x0 >= p->width)
	{
		printf("error: line: x0 out of bounds\n");
		return;
	}

	if (y0 < 0 || y0 >= p->height)
	{
		printf("error: line: y0 out of bounds\n");
		return;
	}

	if (x1 < 0 || x1 >= p->width)
	{
		printf("error: line: x1 out of bounds\n");
		return;
	}

	if (y1 < 0 || y1 >= p->height)
	{
		printf("error: line: y1 out of bounds\n");
		return;
	}

	int dx = x1 - x0;
	if (dx < 0) { dx = -dx; }
    int sx = (x0 < x1)? 1 : -1;
    int dy = y1 - y0;
	if (dy > 0) { dy = -dy; }
    int sy = (y0 < y1)? 1 : -1;
    int error = dx + dy;
	int e2;
    while (1)
	{
		// Plot (x0, y0)
		int i = x0 + y0 * p->width;
		assert(0 <= i && i < p->width * p->height);
		p->colors[i] = val;

        if (x0 == x1 && y0 == y1) { break; }
        e2 = 2 * error;
        if (e2 >= dy)
		{
            if (x0 == x1) { break; }
            error = error + dy;
            x0 = x0 + sx;
		}
        if (e2 <= dx)
		{
            if (y0 == y1) { break; }
            error = error + dx;
            y0 = y0 + sy;
		}
	}
}

// ( width height -- imgID )
void img_alloc(void)
{
	int height = dpop();
	int width = dpop();
	int size = width * height;

	struct Image *new = malloc(sizeof(*new));
	new->width = width;
	new->height = height;
	new->colors = malloc(size * sizeof(*new->colors));

	dpush(ImageAdd(new));
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

	// Create name
	char name[100];
	snprintf(name, sizeof(name), "img-%d.png", suffix);

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

// ( name -- img )
void img_load(void)
{
	int name = dpop();

	char fname[100];
	snprintf(fname, sizeof(fname), "img-%d.png", name);

	// Load PNG image file
	const int requiredN = 4;
	int width, height, n;
	unsigned char *data = stbi_load(fname, &width, &height, &n, requiredN);
	if (!data)
	{
		printf("could not load image file \"%s\"\n", fname);
		dpush(-1);
		return;
	}

	// Alloc new image struct
	struct Image *new = malloc(sizeof(*new));
	assert(new);
	int size = width * height;
	new->width = width;
	new->height = height;
	new->colors = malloc(size * sizeof(*new->colors));
	assert(new->colors);

	// Copy image data
	int *dataI = (int *)data;
	for (int i = 0; i < size; i++)
	{
		new->colors[i] = dataI[i];
	}
	// Done with loaded data
	stbi_image_free(data);

	dpush(ImageAdd(new));
	return;
}

void bye(void)
{
	printf("bye\n");
	exit(0);
}

void quote_code(void)
{
	// Skip opening bracket
	prog++;

	const char *quote_begin = prog;
	while (*prog && *prog != ']')
	{
		prog++;
	}

	if (*prog == ']')
	{
		// Skip closing bracket
		prog++;

		// TODO: free this allocated quote at the end of the program
		int len = prog - quote_begin - 1;
		struct CodeQuote *new = malloc(sizeof(*new));
		new->length = len;
		new->start = quote_begin;
		int index = arrlen(quotesArr);
		arrpush(quotesArr, new);
		dpush(index);
	}
	else
	{
		printf("quote error\n");
		dpush(-1);
	}
}

void do_quote(void)
{
	int quote = dpop();
	if (is_quote(quote))
	{
		const char *save_prog = prog;
		struct CodeQuote *p = quotesArr[quote];
		runScript(p->length, p->start, &dict);
		prog = save_prog;
	}
}

// ( img x0 y0 w h -- img ) crop image to rect
void img_crop(void)
{
	int h = dpop();
	int w = dpop();
	int y0 = dpop();
	int x0 = dpop();
	int img = dtop();

	if (!is_img(img))
	{
		printf("resize: not an image: %d\n", img);
		return;
	}
	struct Image *p = imagesArr[img];

	if (x0 < 0 || y0 < 0
			|| x0 >= p->width || y0 >= p->height
			|| h <= 0 || w <= 0)
	{
		printf("resize: invalid region rectangle\n");
		return;
	}

	// Create new colors array
	int size = w * h;
	int *newColors = malloc(size * sizeof(*newColors));
	// Copy the pixels from img into the new colors
	int i, val;
	for (int x = 0; x < w && (x0 + x) < p->width; x++)
	{
		for (int y = 0; y < h && (y0 + y) < p->height; y++)
		{
			i = (x0 + x) + (y0 + y) * p->width; // index into img
			val = p->colors[i];
			i = x + y * w; // index into newColors
			newColors[i] = val;
		}
	}

	// Replace the image's colors
	free(p->colors);
	p->colors = newColors;
	// Set the image's size
	p->width = w;
	p->height = h;
}

// ( img1 img2 x0 y0 -- img1 ) blit img2 onto img1
void img_blit(void)
{
	int y0 = dpop();
	int x0 = dpop();
	int img2 = dpop();
	int img1 = dtop();

	if (!is_img(img2))
	{
		printf("blit: img2 not an image\n");
		return;
	}
	if (!is_img(img1))
	{
		printf("blit: img1 not an image\n");
		return;
	}

	struct Image *p1 = imagesArr[img1];
	struct Image *p2 = imagesArr[img2];

	int i, val;
	for (int x = 0; x < p2->width && (x0 + x) < p1->width; x++)
	{
		for (int y = 0; y < p2->height && (y0 + y) < p1->height; y++)
		{
			i = x + y * p2->width;
			val = p2->colors[i];
			i = (x0 + x) + (y0 + y) * p1->width;
			p1->colors[i] = val;
		}
	}
}

// ( img1 img2 -- flag )
void img_equal(void)
{
	int img2 = dpop();
	int img1 = dpop();

	if (!is_img(img2))
	{
		printf("img= error: img2 not an image\n");
		dpush(0);
	}

	if (!is_img(img1))
	{
		printf("img= error: img1 not an image\n");
		dpush(0);
	}

	// Images equal themselves
	if (img1 == img2)
	{
		dpush(1);
		return;
	}

	struct Image *p1 = imagesArr[img1];
	struct Image *p2 = imagesArr[img2];

	// Compare sizes
	if (p1->height != p2->height || p1->width != p2->width)
	{
		dpush(0);
		return;
	}

	// Compare contents
	int size = p1->width * p1->height;
	for (int i = 0; i < size; i++)
	{
		if (p1->colors[i] != p2->colors[i])
		{
			dpush(0);
			return;
		}
	}

	// Equal
	dpush(1);
	return;
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

	{ 3, "bye",   bye,       0, 0 },
	{ 2, ".s",    dispstack, 0, 0 },
	{ 1, ".",     dprint,    1, 0 },
	{ 5, "space", space,     0, 0 },
	{ 4, "emit",  emit,      1, 0 },

	{ 1, "[",     quote_code, 0, 1 }, // ( -- codequote )
	{ 2, "do",    do_quote,   1, 0 }, // ( codequote -- )

	{ 3, "rgb",        rgb,    3, 1 }, // ( r g b -- rgba )
	{ 4, "rgba",       rgba,   4, 1 }, // ( r g b a -- rgba )
	{ 5, "value",      value,  1, 1 }, // ( val -- rgba )
	{ 10, "rgb.invert", rgb_invert, 1, 1 }, // ( rgba1 -- rgba2 ) invert rgb values

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
	{ 4, "save",    img_save,   2, 1 }, // ( img name -- img ) name is an int to append to filename
	{ 4, "load",    img_load,   1, 1 }, // ( name -- img ) name is an int to append to filename
	{ 4,"rect",     img_rect,   5, 1 }, // ( img x0 y0 w h val -- img ) draw rectangle
	{ 8,"fillrect",img_fillrect,5, 1 }, // ( img x0 y0 w h val -- img ) fill rectangle
	{ 4,"line",     img_line,   6, 1 }, // ( img x0 y0 x1 y1 val -- img ) draw line
	{ 4, "crop",    img_crop,   5, 1 }, // ( img x0 y0 w h -- img ) crop image to rect
	{ 4, "blit",    img_blit,   4, 1 }, // ( img1 img2 x0 y0 -- img1 ) blit img2 onto img1
	{ 4, "img=",    img_equal,  2, 1 }, // ( img1 img2 -- flag ) see if 2 images have same data
};
struct WordDict dict =
{
	.len = sizeof(words)/sizeof(words[0]),
	.words = words,
};

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("error: missing required argument: file\n");
		return 1;
	}

	const char *fname = argv[1];
	FILE *fp = fopen(fname, "r");
	if (!fp)
	{
		printf("error: could not open \"%s\"\n", fname);
		return 1;
	}

	fseek(fp, 0, SEEK_END); // seek to end of file
	int size = ftell(fp); // get current file pointer
	fseek(fp, 0, SEEK_SET); // seek back to beginning of file

	char *script = malloc(size);
    int num = fread(script, 1, size, fp);
	fclose(fp);

	int code = 1;
	if (num == size)
	{
		code = runScript(size, script, &dict);
		if (code)
		{
			printf("error: %d: %s", code, errMessage(code));
			if (prog && code == ERROR_WORD_NAME)
			{
				const char *wstart;
				int wlen;
				word(prog, &wstart, &wlen);
				printf(": %.*s\n", wlen, wstart);
			}
			else
			{
				printf("\n");
			}
		}
	}
	free(script);
	return code;
}

