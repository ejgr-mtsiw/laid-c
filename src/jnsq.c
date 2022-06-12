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
void set_jnsq_bits(unsigned long *line, unsigned int inconsistency,
		const unsigned int n_attributes, const unsigned int n_longs,
		const unsigned int n_bits_for_class) {

	// Check how many attributes remain
	unsigned char remaining_attributes = n_attributes % BLOCK_BITS;

	// Jnsq starts after this bit
	unsigned char jnsq_start = BLOCK_BITS - remaining_attributes;

	unsigned long last_long = line[n_longs - 1];

	last_long >>= jnsq_start;

	unsigned char i = 0;

	for (i = 0; inconsistency > 0 && i < n_bits_for_class; i++) {
		last_long <<= 1;

		if (inconsistency & 1) {
			last_long |= 1;
		}

		inconsistency >>= 1;
	}

	last_long <<= jnsq_start - i;

	line[n_longs - 1] = last_long;
}

/**
 * Compares 2 lines and updates jnsq on to_update if needed and updates
 * inconsistency level
 */
void update_jnsq(unsigned long *to_update, const unsigned long *to_compare,
		unsigned int *inconsistency, const unsigned int n_attributes,
		const unsigned int n_longs, const unsigned int n_bits_for_class) {

	// Set the line JNSQ
	set_jnsq_bits(to_update, (*inconsistency), n_attributes, n_longs,
			n_bits_for_class);

	if (has_same_attributes(to_update, to_compare, n_attributes, n_longs)) {
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
unsigned int add_jnsqs(dataset_t *dataset) {

	// Current line
	unsigned long *current = dataset->data;

	// Previous line
	unsigned long *prev = current;

	// Number of attributes
	unsigned int n_attributes = dataset->n_attributes;

	// Number of longs in a line
	unsigned int n_longs = dataset->n_longs;

	// Number of observations in the dataset
	unsigned int n_observations = dataset->n_observations;

	// Number of bits needed to store class
	unsigned int n_bits_for_class = dataset->n_bits_for_class;

	// Last line
	unsigned long *last = GET_LAST_OBSERVATION(dataset->data, n_observations,
			n_longs);

	// Inconsistency
	unsigned int inconsistency = 0;

	// Max inconsistency found
	unsigned int max_inconsistency = 0;

	// first line has jnsq=0
	set_jnsq_bits(current, 0, n_attributes, n_longs, n_bits_for_class);

	// Now do the others
	for (prev = dataset->data; prev < last; NEXT_LINE(prev, n_longs)) {
		NEXT_LINE(current, n_longs);

		if (has_same_attributes(current, prev, n_attributes, n_longs)) {
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
		set_jnsq_bits(current, inconsistency, n_attributes, n_longs,
				n_bits_for_class);
	}

	// Update number of attributes to include the new JNSQs
	if (max_inconsistency > 0) {
		// How many bits are needed for jnsq attributes
		unsigned int n_bits_for_jnsq = ceil(log2(max_inconsistency + 1));

		dataset->n_attributes += n_bits_for_jnsq;
		dataset->n_bits_for_jnsqs = n_bits_for_jnsq;
	}

	return max_inconsistency;
}
