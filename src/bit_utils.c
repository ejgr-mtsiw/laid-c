/*
 ============================================================================
 Name        : bit_utils.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manipulate bits
 ============================================================================
 */

#include "bit_utils.h"

/**
 * An interesting problem I've been pondering the past few days is how to copy
 * one integer's bits into another integer at a given position in the
 * destination integer. So, for example, given the destination integer
 * 0xdeadbeef and the source integer 0xabcd, the idea would be to get a result
 * of 0xabcdbeef (given a destination position of 16 bits) or 0xdeabcdef (given
 * a destination position of 8 bits). With the arbitrary limitation of avoiding
 * conditionals or loops (allowing myself to use just mathematical/bitwise
 * operations), I developed the following function (C++)
 * https://stackoverflow.com/questions/1283221/algorithm-for-copying-n-bits-at-arbitrary-position-from-one-int-to-another
 */
uint_fast64_t set_bits(const uint_fast64_t destination,
		const uint_fast64_t source, const uint_fast8_t at,
		const uint_fast8_t numbits) {
	uint_fast64_t mask =
			((~0LU) >> (sizeof(uint_fast64_t) * CHAR_BIT - numbits)) << at;
	return (destination & ~mask) | ((source << at) & mask);
}

uint_fast64_t get_bits(const uint_fast64_t source, const uint_fast8_t at,
		const uint_fast8_t numbits) {
	uint_fast64_t mask = (~0LU) << numbits;
	return (uint_fast64_t) ((source >> at) & ~mask);
}
