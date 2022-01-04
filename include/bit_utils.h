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

/**
 *
 */
#define CHECK_BIT(var,pos) !!(((var) >> (pos)) & 1)

/**
 * https://stackoverflow.com/questions/1283221/algorithm-for-copying-n-bits-at-arbitrary-position-from-one-int-to-another
 */
unsigned long set_bits(const unsigned long destination, const unsigned long source, const unsigned int at,
		const unsigned int numbits);

unsigned long get_bits(const unsigned long source, const unsigned int at, const unsigned int numbits);

#endif
