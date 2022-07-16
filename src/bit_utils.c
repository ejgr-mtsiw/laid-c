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
