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
unsigned long setbits(unsigned long destination, unsigned long source, unsigned int at, unsigned int numbits);

#endif
