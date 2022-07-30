/*
 ============================================================================
 Name        : dataset.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage datasets
 ============================================================================
 */

#include "dataset.h"

/**
 * Initializes a dataset structure
 */
void init_dataset(dataset_t *dataset) {
	// Store data
	dataset->data = NULL;
	dataset->n_observations_per_class = NULL;
	dataset->observations_per_class = NULL;
	dataset->n_attributes = 0;
	dataset->n_bits_for_class = 0;
	dataset->n_bits_for_jnsqs = 0;
	dataset->n_classes = 0;
	dataset->n_observations = 0;
	dataset->n_words = 0;
}

uint32_t get_class(const word_t *line, const uint32_t n_attributes,
		const uint32_t n_words, const uint8_t n_bits_for_class) {

	// How many attributes remain on last word
	uint8_t remaining_attributes = n_attributes % WORD_BITS;

	if (n_bits_for_class == 1
			|| (remaining_attributes + n_bits_for_class <= WORD_BITS)) {
		// All bits on same word
		// Class starts here
		uint8_t at = (uint8_t) (WORD_BITS - remaining_attributes
				- n_bits_for_class);

		return (uint32_t) get_bits(line[n_words - 1], at, n_bits_for_class);
	}

	// Class bits are split between 2 words

	// n bits on penultimate word
	uint8_t n_bits_p = WORD_BITS - remaining_attributes;

	// n bits on last word
	uint8_t n_bits_l = n_bits_for_class - n_bits_p;

	//bits on penultimate word
	uint32_t high_bits = (uint32_t) get_bits(line[n_words - 2], 0, n_bits_p);
	//bits on last word
	uint32_t low_bits = (uint32_t) get_bits(line[n_words - 1],
	WORD_BITS - n_bits_l, n_bits_l);

	// Merge bits
	high_bits <<= n_bits_l;
	high_bits |= low_bits;

	return high_bits;
}

/**
 * Compares two lines of the dataset
 * Used to sort the dataset
 */
int compare_lines_extra(const void *a, const void *b, void *n_words) {

	const word_t *ula = (const word_t*) a;
	const word_t *ulb = (const word_t*) b;
	word_t va = 0;
	word_t vb = 0;

	uint32_t n_l = *(uint32_t*) n_words;

	for (uint32_t i = 0; i < n_l; i++) {

		va = ula[i];
		vb = ulb[i];

		if (va > vb) {
			return 1;
		}

		if (va < vb) {
			return -1;
		}
	}

	return 0;
}

/**
 * Checks if the lines have the same attributes
 */
bool has_same_attributes(const word_t *a, const word_t *b,
		const uint32_t n_attributes) {

	// How many words for attributes?
	uint32_t n_words = (uint32_t) (n_attributes / WORD_BITS)
			+ (n_attributes % WORD_BITS != 0);

	// How many attributes remain on last word
	uint8_t remaining_attributes = n_attributes % WORD_BITS;

	// Check full words
	for (uint32_t i = 0; i < n_words - !!remaining_attributes; i++) {
		if (a[i] != b[i]) {
			return false;
		}
	}

	if (remaining_attributes == 0) {
		// Nothing more to check
		return true;
	}

	// We need to check last word
	word_t last_word = get_bits((a[n_words - 1] ^ b[n_words - 1]),
	WORD_BITS - remaining_attributes, remaining_attributes);

	return (last_word == 0);
}

uint32_t remove_duplicates(dataset_t *dataset) {

	word_t *line = dataset->data;
	word_t *last = line;

	uint32_t n_words = dataset->n_words;
	uint32_t n_observations = dataset->n_observations;
	uint32_t n_uniques = 1;

	for (uint32_t i = 0; i < n_observations - 1; i++) {
		NEXT_LINE(line, n_words);
		if (compare_lines_extra(line, last, &n_words) != 0) {
			NEXT_LINE(last, n_words);
			n_uniques++;
			if (last != line) {
				memcpy(last, line, sizeof(word_t) * n_words);
			}
		}
	}

	// Update number of observations
	dataset->n_observations = n_uniques;
	return n_observations - n_uniques;
}

/**
 * Fill the arrays with the number os items per class and also a matrix with
 * references to the lines that belong to each class to simplify the
 * calculation of the disjoint matrix
 */
oknok_t fill_class_arrays(dataset_t *dataset) {

	/**
	 * Array that stores the number of observations for each class
	 */
	dataset->n_observations_per_class = (uint32_t*) calloc(dataset->n_classes,
			sizeof(uint32_t));
	if (dataset->n_observations_per_class == NULL) {
		fprintf(stderr, "Error allocating n_observations_per_class\n");
		return NOK;
	}

	/**
	 * Matrix that stores the list of observations per class
	 */
	// WHATIF: should we replace the static matrix by pointers so each
	// class has n items? Right now we waste at least half the matrix space
	// WHATIF: If we reduce the number of possible columns and lines to
	// 2^32-1 we could use half the memory by storing the line indexes
	dataset->observations_per_class = (word_t**) calloc(
			dataset->n_classes * dataset->n_observations, sizeof(word_t*));
	if (dataset->observations_per_class == NULL) {
		fprintf(stderr, "Error allocating observations_per_class\n");
		return NOK;
	}

	// Current line
	word_t *line = dataset->data;

	// This line class
	uint32_t line_class = 0;

	// Number of longs in a line
	uint32_t n_words = dataset->n_words;

	// Number of attributes
	uint32_t n_attributes = dataset->n_attributes;

	// Number of observations
	uint32_t n_observations = dataset->n_observations;

	// Number of bits needed to store class
	uint8_t n_bits_for_class = dataset->n_bits_for_class;

	// Observations per class
	word_t **observations_per_class = dataset->observations_per_class;

	// Number of observations per class
	uint32_t *n_observations_per_class = dataset->n_observations_per_class;

	for (uint32_t i = 0; i < n_observations; i++) {
		line_class = get_class(line, n_attributes, n_words, n_bits_for_class);

		observations_per_class[line_class * n_observations
				+ n_observations_per_class[line_class]] = line;

		n_observations_per_class[line_class]++;

		NEXT_LINE(line, n_words);
	}

	return OK;
}

void print_dataset_details(FILE *stream, const dataset_t *dataset) {
	fprintf(stream, "Dataset:\n");
	fprintf(stream, " - classes = %d ", dataset->n_classes);
	fprintf(stream, "[%d bits]\n", dataset->n_bits_for_class);
	fprintf(stream, " - attributes = %d \n", dataset->n_attributes);
	fprintf(stream, " - observations = %d \n", dataset->n_observations);
}

void free_dataset(dataset_t *dataset) {
	free(dataset->data);
	free(dataset->n_observations_per_class);
	free(dataset->observations_per_class);

	dataset->data = NULL;
	dataset->n_observations_per_class = NULL;
	dataset->observations_per_class = NULL;
}
