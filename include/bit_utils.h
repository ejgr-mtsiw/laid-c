/*
 ============================================================================
 Name        : util.h
 Author      : Eduardo Ribeiro
 Description : Utils
 ============================================================================
 */

#ifndef BIT_UTILS_H
#define BIT_UTILS_H

#include <limits.h>

/**
 * How many bits in a long?
 */
#define LONG_BITS (CHAR_BIT * sizeof (unsigned long))

/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1UL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1UL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1UL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1UL<<(b)))) // '!!' to make sure this returns 0 or 1

#define BITMASK_SET(x, mask) ((x) |= (mask))
#define BITMASK_CLEAR(x, mask) ((x) &= (~(mask)))
#define BITMASK_FLIP(x, mask) ((x) ^= (mask))
#define BITMASK_CHECK_ALL(x, mask) (!(~(x) & (mask)))
#define BITMASK_CHECK_ANY(x, mask) ((x) & (mask))

/**
 * https://stackoverflow.com/questions/1283221/algorithm-for-copying-n-bits-at-arbitrary-position-from-one-int-to-another
 */
unsigned long set_bits(const unsigned long destination, const unsigned long source, const unsigned int at,
		const unsigned int numbits);

/**
 *
 */
unsigned long get_bits(const unsigned long source, const unsigned int at, const unsigned int numbits);

#endif
