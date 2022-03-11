#ifndef __COMSCRIPT_H
#define __COMSCRIPT_H

#define DATA_STACK_SZ 100

// Word lookup entry
struct WordLookup
{
	int len; // length of the name
	const char *name; // name of the word
	void (*func)(void); // function to call to execute it
	int numInputs; // number of word's inputs from stack
	int numOutputs; // number of word's outputs to stack
};

// Word dictionary
struct WordDict
{
	int len; // length of the words array
	struct WordLookup *words; // array of word-lookups
};

int dstack[DATA_STACK_SZ]; // Data stack
int dI; // Data stack index

int runScript(int len, const char *str, struct WordDict *dict);

int dpop(void);    // Pop the top int from the stack and return it.
void dpush(int);   // Push an int onto the stack.
void dreset(void); // Reset stack to empty.

int find(struct WordDict *dict, int wordLen, const char *word);
int number(int len, const char *str);
void word(const char *in, const char **start, int *len);

void add(void);       // 2 -> 1 ( x y -- z )
void subtract(void);  // 2 -> 1 ( x y -- z )
void multiply(void);  // 2 -> 1 ( x y -- z )
void divide(void);    // 2 -> 1 ( x y -- z )
void drop(void);      // 1 -> 0 ( x -- )
void duplicate(void); // 1 -> 2 ( x -- x x )
void swap(void);      // 2 -> 2 ( x y -- y x )

#endif /* __COMSCRIPT_H */

#ifdef COMSCRIPT_IMPLEMENTATION

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

// data stack and index
int dstack[DATA_STACK_SZ];
int dI = 0;

// Internal for runScript
static int isRunning = 1;

void dpush(int n)
{
	assert(dI < DATA_STACK_SZ);
	dstack[dI] = n;
	dI++;
}

int dpop(void)
{
	assert(dI > 0);
	dI--;
	return dstack[dI];
}

int number(int len, const char *str)
{
	int n = 0;
	int i = 0;
	while(i < len && isdigit(str[i]))
	{
		n = n * 10 + (str[i] - '0');
		i++;
	}
	return n;
}

// (in) -> (start, len)
void word(const char *in, const char **start, int *len)
{
	int i = 0;
	// Skip whitespace
	while(in[i] && isspace(in[i]))
	{
		i++;
	}
	*start = &in[i];
	int startI = i;
	// Read non-whitespace
	while (in[i] && !isspace(in[i]))
	{
		i++;
	}
	*len = i - startI;
}

static int streq(const char *a, const char *b, int len)
{
	for (int j = 0; j < len; j++)
	{
		if (a[j] != b[j])
		{
			return 0;
		}
	}
	return 1;
}

void add(void) { dpush(dpop() + dpop()); }
void subtract(void)
{
	int a = dpop();
	dpush(dpop() - a);
}
void multiply(void) { dpush(dpop() * dpop()); }
void divide(void)
{
	int a = dpop();
	dpush(dpop() / a);
}
void drop(void) { dpop(); }
void duplicate(void) { dpush(dstack[dI - 1]); }

// Returns index into words, or -1 if not found.
int find(struct WordDict *dict, int wordLen, const char *word)
{
	if (!dict || !word)
	{
		return -1;
	}

	for (int i = 0; i < dict->len; i++)
	{
		struct WordLookup record = dict->words[i];
		if (record.len != wordLen)
		{
			continue;
		}
		if (!streq(record.name, word, wordLen))
		{
			continue;
		}
		return i;
	}
	return -1;
}

int runScript(int len, const char *str, struct WordDict *dict)
{
	const char *prog = str;
	const char *wstart = NULL; // word start
	int wlen = 0; // word length
	while (isRunning)
	{
		// Read a word from the program string;
		word(prog, &wstart, &wlen);
		if (!wlen)
		{
			// End of input
			break;
		}

		// Lookup the word.
		int i = find(dict, wlen, wstart);
		if (i >= 0)
		{
			// Found the word in the list. Try to execute it.
			struct WordLookup w = dict->words[i];
			// Check number of arguments on the stack
			if (w.numInputs > dI)
			{
				// Not enough values on stack
				return 1;
			}
			else if (w.numOutputs - w.numInputs > DATA_STACK_SZ - dI)
			{
				// Too many values on the stack
				return 1;
			}
			else
			{
				// Execute it.
				w.func();
			}
		}
		else if (isdigit(*wstart))
		{
			// Number.
			if (dI < DATA_STACK_SZ)
			{
				dpush(number(wlen, wstart));
			}
			else
			{
				// too many items on stack
				return 1;
			}
		}
		else
		{
			// unknown word
			return 1;
		}

		// Next word in program string.
		prog = wstart + wlen;
	}
	return 0;
}

#endif /* COMSCRIPT_IMPLEMENTATION */

