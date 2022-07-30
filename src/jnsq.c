/*
 ============================================================================
 Name        : jnsq.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage JNSQ
 ============================================================================
 */

#include "jnsq.h"

void set_jnsq_bits(word_t *line, uint32_t inconsistency,
		const uint32_t n_attributes, const uint32_t n_words,
		const uint8_t n_bits_for_class) {

	// How many attributes remain on last word
	uint8_t remaining_attributes = n_attributes % WORD_BITS;

	if (remaining_attributes + n_bits_for_class > WORD_BITS) {
		// jnsq bits split between words

		// n jnsq bits on penultimate word
		uint8_t n_bits_p = WORD_BITS - remaining_attributes;

		// There's no attributes on last word
		remaining_attributes = 0;

		word_t penultimate_word = line[n_words - 2];

		penultimate_word >>= n_bits_p;

		for (; n_bits_p > 0; n_bits_p--) {
			penultimate_word <<= 1;
			penultimate_word |= (inconsistency & 1U);
			inconsistency >>= 1;
		}

		line[n_words - 2] = penultimate_word;
	}

	// All remaining jnsq bits in the same word
	word_t last_word = line[n_words - 1];

	uint8_t jnsq_start = WORD_BITS - remaining_attributes;
	last_word >>= jnsq_start;

	while (inconsistency) {
		last_word <<= 1;
		last_word |= (inconsistency & 1U);
		inconsistency >>= 1;
		jnsq_start--;
	}

	last_word <<= jnsq_start;
	line[n_words - 1] = last_word;
}

void update_jnsq(word_t *to_update, const word_t *to_compare,
		uint32_t *inconsistency, const uint32_t n_attributes,
		const uint32_t n_words, const uint8_t n_bits_for_class) {

	// Set the line JNSQ
	set_jnsq_bits(to_update, (*inconsistency), n_attributes, n_words,
			n_bits_for_class);

	if (has_same_attributes(to_update, to_compare, n_attributes)) {
		// Inconsistency!
		(*inconsistency)++; //Because observations are sorted by class
	} else {
		// Differente attributes - reset JNSQ
		(*inconsistency) = 0;
	}
}

/**
 * Adds the JNSQs attributes to the dataset
 */
uint32_t add_jnsqs(dataset_t *dataset) {

	// Current line
	word_t *current = dataset->data;

	// Previous line
	word_t *prev = current;

	// Number of attributes
	uint32_t n_attributes = dataset->n_attributes;

	// Number of longs in a line
	uint32_t n_words = dataset->n_words;

	// Number of observations in the dataset
	uint32_t n_observations = dataset->n_observations;

	// Number of bits needed to store class
	uint8_t n_bits_for_class = dataset->n_bits_for_class;

	// Last line
	word_t *last = GET_LAST_OBSERVATION(dataset->data, n_observations, n_words);

	// Inconsistency
	uint32_t inconsistency = 0;

	// Max inconsistency found
	uint32_t max_inconsistency = 0;

	// first line has jnsq=0
	set_jnsq_bits(current, 0, n_attributes, n_words, n_bits_for_class);

	// Now do the others
	for (prev = dataset->data; prev < last; NEXT_LINE(prev, n_words)) {
		NEXT_LINE(current, n_words);

		if (has_same_attributes(current, prev, n_attributes)) {
			// Inconsistency!
			inconsistency++; //Because observations are sorted by class

			// Update max
			if (inconsistency > max_inconsistency) {
				max_inconsistency = inconsistency;
			}
		} else {
			// Differente attributes - reset JNSQ
			inconsistency = 0;
		}

		// Set the line JNSQ
		set_jnsq_bits(current, inconsistency, n_attributes, n_words,
				n_bits_for_class);
	}

	// Update number of attributes to include the new JNSQs
	if (max_inconsistency > 0) {
		// How many bits are needed for jnsq attributes
		uint8_t n_bits_for_jnsq = ceil(log2(max_inconsistency + 1));

		dataset->n_attributes += n_bits_for_jnsq;
		dataset->n_bits_for_jnsqs = n_bits_for_jnsq;
	}

	return max_inconsistency;
}
