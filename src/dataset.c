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
 * Prints a line to stream
 */
//void print_line(FILE *stream, const unsigned long *line,
//		const unsigned char extra_bits) {
//
//	// Current attribute
//	unsigned int columns_to_write = g_n_attributes;
//
//	if (extra_bits == 1) {
//		columns_to_write += g_n_bits_for_class;
//	}
//
//	for (unsigned int i = 0; i < g_n_longs && columns_to_write > 0; i++) {
//		for (int_fast8_t j = BLOCK_BITS - 1; j >= 0 && columns_to_write > 0;
//				j--) {
//			if (j % 8 == 0) {
//				fprintf(stream, " ");
//			}
//
//			fprintf(stream, "%d", BIT_CHECK(line[i], j));
//
//			columns_to_write--;
//		}
//
//		if (i == 0 && g_n_longs > 2) {
//			// Too many to write, let's jump to the end
//			i = g_n_longs - 2;
//			fprintf(stream, " ... ");
//		}
//	}
//
//	//fprintf(stream, "\n");
//}
/**
 * Prints the whole dataset
 */
//void print_dataset(FILE *stream, const char *title, unsigned long *dataset,
//		const unsigned char extra_bits) {
//
//	fprintf(stream, "%s\n", title);
//
//	unsigned long *line = dataset;
//
//	for (unsigned int i = 0; i < g_n_observations; i++) {
//		print_line(stream, line, extra_bits);
//		fprintf(stream, "\n");
//		NEXT_LINE(line);
//
//		if (g_n_observations > 20 && i == 9) {
//			fprintf(stream, " ... \n");
//			//skip
//			i = g_n_observations - 10;
//			line = dataset + (g_n_observations - 10) * g_n_longs;
//		}
//	}
//}
///**
// * Compares two lines of the dataset
// * Used to sort the dataset
// */
//int compare_lines(const void *a, const void *b) {
//
//	const unsigned long *ula = (const unsigned long*) a;
//	const unsigned long *ulb = (const unsigned long*) b;
//	unsigned long va = 0;
//	unsigned long vb = 0;
//
//	for (unsigned int i = 0; i < g_n_longs; i++) {
//
//		va = ula[i];
//		vb = ulb[i];
//
//		if (va > vb) {
//			return 1;
//		}
//
//		if (va < vb) {
//			return -1;
//		}
//	}
//
//	return 0;
//}
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
