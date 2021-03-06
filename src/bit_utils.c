/*
 ============================================================================
 Name        : bit_utils.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manipulate bits
 ============================================================================
 */

#include "bit_utils.h"

word_t set_bits(const word_t destination, const word_t source, const uint8_t at,
		const uint8_t numbits) {
	word_t mask = (((word_t) (~0LU)) >> (WORD_BITS - numbits)) << at;
	return (destination & ~mask) | ((source << at) & mask);
}

word_t get_bits(const word_t source, const uint8_t at, const uint8_t numbits) {
	word_t mask = ((word_t) (~0LU)) << numbits;
	return (source >> at) & ~mask;
}

void transpose64(uint64_t a[64]) {
	int j, k;
	uint64_t m, t;

	for (j = 32, m = 0x00000000FFFFFFFF; j; j >>= 1, m ^= m << j) {
		for (k = 0; k < 64; k = ((k | j) + 1) & ~j) {
			t = (a[k] ^ (a[k | j] >> j)) & m;
			a[k] ^= t;
			a[k | j] ^= (t << j);
		}
	}
}
