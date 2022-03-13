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
#include <stdint.h>

static const uint_fast64_t AND_MASK_TABLE[64] = { 0x1, 0x2, 0x4, 0x8, 0x10,
		0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000,
		0x8000, 0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000,
		0x400000, 0x800000, 0x1000000, 0x2000000, 0x4000000, 0x8000000,
		0x10000000, 0x20000000, 0x40000000, 0x80000000, 0x100000000,
		0x200000000, 0x400000000, 0x800000000, 0x1000000000, 0x2000000000,
		0x4000000000, 0x8000000000, 0x10000000000, 0x20000000000, 0x40000000000,
		0x80000000000, 0x100000000000, 0x200000000000, 0x400000000000,
		0x800000000000, 0x1000000000000, 0x2000000000000, 0x4000000000000,
		0x8000000000000, 0x10000000000000, 0x20000000000000, 0x40000000000000,
		0x80000000000000, 0x100000000000000, 0x200000000000000,
		0x400000000000000, 0x800000000000000, 0x1000000000000000,
		0x2000000000000000, 0x4000000000000000, 0x8000000000000000 };

/**
 * How many bits are we using for each block?
 */
#define BLOCK_BITS 64

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
uint_fast64_t set_bits(const uint_fast64_t destination,
		const uint_fast64_t source, const uint_fast8_t at,
		const uint_fast8_t numbits);

/**
 * Return numbits from source starting at at
 */
uint_fast64_t get_bits(const uint_fast64_t source, const uint_fast8_t at,
		const uint_fast8_t numbits);

#endif
