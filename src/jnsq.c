/*
 ============================================================================
 Name        : jnsq.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage JNSQ
 ============================================================================
 */

#include "jnsq.h"

/**
 * Replaces the class bits with jnsq bits
 * It's ok, because the number of jnsq bits is always <= bits needed for class
 * And we don't need the class anymore because we extracted it to the
 * dataset_line structure.
 * And we fill the bits in reverse jnsq=1 with 3bits = 1 0 0
 * So the extra bits are zeroed and can be ignored in the calculations
 *
 * Also inconsistency = class
 * See Apolónia, J., & Cavique, L. (2019). Seleção de Atributos de Dados
 * Inconsistentes em ambiente HDF5+ Python na cloud INCD. Revista de
 * Ciências da Computação, 85-112.
 */
void set_jnsq_bits(uint_fast64_t *line, uint_fast8_t inconsistency) {

	// Check how many attributes remain
	uint_fast8_t remaining_attributes = g_n_attributes % BLOCK_BITS;

	// Jnsq starts after this bit
	uint_fast8_t jnsq_start = BLOCK_BITS - remaining_attributes;

	uint_fast64_t last_long = line[g_n_longs - 1];

	last_long >>= jnsq_start;

	uint_fast8_t i = 0;

	for (i = 0; inconsistency > 0 && i < g_n_bits_for_class; i++) {
		last_long <<= 1;

		if (inconsistency & 1) {
			last_long |= 1;
		}

		inconsistency >>= 1;
	}

	last_long <<= jnsq_start - i;

	line[g_n_longs - 1] = last_long;
}

/**
 * Compares 2 lines and updates jnsq on to_update if needed and updates
 * inconsistency level
 */
void update_jnsq(uint_fast64_t *to_update, const uint_fast64_t *to_compare,
		uint_fast8_t *inconsistency) {

	// Set the line JNSQ
	set_jnsq_bits(to_update, (*inconsistency));

	if (has_same_attributes(to_update, to_compare)) {
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
uint_fast8_t add_jnsqs(uint_fast64_t *dataset) {

	// Current line
	uint_fast64_t *current = dataset;

	// Previous line
	uint_fast64_t *prev = dataset;

	// Last line
	uint_fast64_t *last = GET_LAST_OBSERVATION(dataset);

	// Inconsistency
	uint_fast8_t inconsistency = 0;

	// Max inconsistency found
	uint_fast8_t max_inconsistency = 0;

	// first line has jnsq=0
	set_jnsq_bits(current, 0);

	// Now do the others
	for (prev = dataset; prev < last; NEXT_LINE(prev)) {
		NEXT_LINE(current);

		if (has_same_attributes(current, prev)) {
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
		set_jnsq_bits(current, inconsistency);
	}

	return max_inconsistency;
}
