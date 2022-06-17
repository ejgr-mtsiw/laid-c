/*
 ============================================================================
 Name        : dataset.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage datasets
 ============================================================================
 */

#include "dataset.h"

/**
 * Returns the class of this data line
 */
unsigned int get_class(const unsigned long *line,
		const unsigned int n_attributes, const unsigned int n_longs,
		const unsigned int n_bits_for_class) {

	// Check how many attributes remain on last long
	unsigned char remaining_attributes = n_attributes % BLOCK_BITS;

	// Class starts here
	unsigned char at = BLOCK_BITS - remaining_attributes - n_bits_for_class;

	return (unsigned char) get_bits(line[n_longs - 1], at, n_bits_for_class);
}

/**
 * Compares two lines of the dataset
 * Used to sort the dataset
 */
int compare_lines_extra(const void *a, const void *b, void *n_longs) {

	const unsigned long *ula = (const unsigned long*) a;
	const unsigned long *ulb = (const unsigned long*) b;
	unsigned long va = 0;
	unsigned long vb = 0;

	unsigned int n_l = *(unsigned int*) n_longs;

	for (unsigned int i = 0; i < n_l; i++) {

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
bool has_same_attributes(const unsigned long *a, const unsigned long *b,
		const unsigned int n_attributes, const unsigned long n_longs) {

	for (unsigned int i = 0; i < n_longs - 1; i++) {
		if (a[i] != b[i]) {
			return false;
		}
	}

	unsigned int remaining_attributes = n_attributes % BLOCK_BITS;

	if (remaining_attributes == 0) {
		// Nothing more to check
		return true;
	}

	unsigned long mask = ~0LU;
	mask >>= remaining_attributes;
	mask = ~mask;

	if ((a[n_longs - 1] & mask) != (b[n_longs - 1] & mask)) {
		return false;
	}

	return true;
}

unsigned int remove_duplicates(dataset_t *dataset) {

	unsigned long *line = dataset->data;
	unsigned long *last = line;

	unsigned int n_longs = dataset->n_longs;
	unsigned int n_observations = dataset->n_observations;
	unsigned int n_uniques = 1;

	for (unsigned int i = 0; i < n_observations - 1; i++) {
		NEXT_LINE(line, n_longs);
		if (compare_lines_extra(line, last, &n_longs) != 0) {
			NEXT_LINE(last, n_longs);
			n_uniques++;
			if (last != line) {
				memcpy(last, line, sizeof(unsigned long) * n_longs);
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
 *
 * Inputs are expected to be zeroed arrays
 */
void fill_class_arrays(dataset_t *dataset) {

	// Current line
	unsigned long *line = dataset->data;

	// This line class
	unsigned char line_class = 0;

	// Number of longs in a line
	unsigned int n_longs = dataset->n_longs;

	// Number of attributes
	unsigned int n_attributes = dataset->n_attributes;

	// Number of observations
	unsigned int n_observations = dataset->n_observations;

	// Number of bits needed to store class
	unsigned int n_bits_for_class = dataset->n_bits_for_class;

	// Observations per class
	unsigned long **observations_per_class = dataset->observations_per_class;

	// Number of observations per class
	unsigned int *n_observations_per_class = dataset->n_observations_per_class;

	for (unsigned int i = 0; i < n_observations; i++) {
		line_class = get_class(line, n_attributes, n_longs, n_bits_for_class);

		observations_per_class[line_class * n_observations
				+ n_observations_per_class[line_class]] = line;

		n_observations_per_class[line_class]++;

		NEXT_LINE(line, n_longs);
	}
}

void print_dataset_details(FILE *stream, const dataset_t *dataset) {
	fprintf(stream, "Dataset:\n");
	fprintf(stream, " - classes = %d ", dataset->n_classes);
	fprintf(stream, "[%d bits]\n", dataset->n_bits_for_class);
	fprintf(stream, " - attributes = %d \n", dataset->n_attributes);
	fprintf(stream, " - observations = %d \n\n", dataset->n_observations);
}

void free_dataset(dataset_t *dataset) {
	free(dataset->data);
	free(dataset->n_observations_per_class);
	free(dataset->observations_per_class);

	dataset->data = NULL;
	dataset->n_observations_per_class = NULL;
	dataset->observations_per_class = NULL;
}
