#ifndef __COMSCRIPT_H
#define __COMSCRIPT_H

#ifndef DATA_STACK_SZ
#define DATA_STACK_SZ 100
#endif /* DATA_STACK_SZ */

#define ERROR_STACK_OVERFLOW  1
#define ERROR_STACK_UNDERFLOW 2
#define ERROR_WORD_NAME       3

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

int runScript(int len, const char *str, struct WordDict *dict); // Interpret the str as a list of words and execute them.

const char *errMessage(int code); // convert runScript error code to message

int dtop(void);    // Return the top of the stack
int dpop(void);    // Pop the top int from the stack and return it.
void dpush(int);   // Push an int onto the stack.
int dpick(int n);  // Pick the Nth item on the stack (starting at 0)
void dreset(void); // Reset stack to empty.

int find(struct WordDict *dict, int wordLen, const char *word); // Return an index into dict or negative if not found
int number(int len, const char *str);
void word(const char *in, const char **start, int *len);

void add(void);       // Stack effect: ( x y -- x+y ),  WordEntry: { 1, "+",    add,       2, 1 }
void subtract(void);  // Stack effect: ( x y -- x-y ),  WordEntry: { 1, "-",    subtract,  2, 1 }
void multiply(void);  // Stack effect: ( x y -- x*y ),  WordEntry: { 1, "*",    multiply,  2, 1 }
void divide(void);    // Stack effect: ( x y -- x/y ),  WordEntry: { 1, "/",    divide,    2, 1 }
void drop(void);      // Stack effect: ( x -- )      ,  WordEntry: { 4, "drop", drop,      1, 0 }
void duplicate(void); // Stack effect: ( x -- x x )  ,  WordEntry: { 3, "dup",  duplicate, 1, 2 }
void swap(void);      // Stack effect: ( x y -- y x ),  WordEntry: { 4, "swap", swap,      2, 2 }
void over(void);      // Stack effect: ( x y -- x y x), WordEntry: { 4, "over",  over,      2, 3 }
void nip(void);       // Stack effect: ( x y -- y ),    WordEntry: { 3, "nip",   nip,       2, 1 }

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

int dtop(void)
{
	return dpick(0);
}

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

int dpick(int n)
{
	assert(dI > n);
	return dstack[dI - n - 1];
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

void add(void)
{
	dpush(dpop() + dpop());
}

void subtract(void)
{
	int a = dpop();
	dpush(dpop() - a);
}

void multiply(void)
{
	dpush(dpop() * dpop());
}

void divide(void)
{
	int a = dpop();
	dpush(dpop() / a);
}

void drop(void)
{
	assert(dI >= 1);
	dpop();
}

void duplicate(void)
{
	dpush(dstack[dI - 1]);
}

void swap(void)
{
	assert(dI >= 2);
	int x = dstack[dI - 2];
	dstack[dI - 2] = dstack[dI - 1];
	dstack[dI - 1] = x;
}

void over(void)
{
	dpush(dpick(1));
}

void nip(void)
{
	assert(dI >= 2);
	int x = dpop();
	dpop();
	dpush(x);
}

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
				return ERROR_STACK_UNDERFLOW;
			}
			else if (w.numOutputs - w.numInputs > DATA_STACK_SZ - dI)
			{
				// Too many values on the stack
				return ERROR_STACK_OVERFLOW;
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
				return ERROR_STACK_OVERFLOW;
			}
		}
		else
		{
			// unknown word
			return ERROR_WORD_NAME;
		}

		// Next word in program string.
		prog = wstart + wlen;
	}
	return 0;
}

// convert runScript error code to message
const char *errMessage(int code)
{
	switch (code)
	{
		case ERROR_STACK_OVERFLOW: return "stack overflow";
		case ERROR_STACK_UNDERFLOW: return "stack underflow";
		case ERROR_WORD_NAME: return "unknown word name";
		default: return "not a runScript error";
	}
}

#endif /* COMSCRIPT_IMPLEMENTATION */

