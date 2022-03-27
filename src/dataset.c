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
uint_fast8_t get_class(const uint_fast64_t *line) {

	// Check how many attributes remain on last long
	uint_fast8_t remaining_attributes = g_n_attributes % BLOCK_BITS;

	// Class starts here
	uint_fast8_t at = BLOCK_BITS - remaining_attributes - g_n_bits_for_class;

	return (uint_fast8_t) get_bits(line[g_n_longs - 1], at, g_n_bits_for_class);
}

/**
 * Prints a line to stream
 */
void print_line(FILE *stream, const uint_fast64_t *line,
		const uint_fast8_t extra_bits) {

	// Current attribute
	uint_fast32_t columns_to_write = g_n_attributes;

	if (extra_bits == 1) {
		columns_to_write += g_n_bits_for_class;
	}

	for (uint_fast32_t i = 0; i < g_n_longs && columns_to_write > 0; i++) {
		for (int_fast8_t j = BLOCK_BITS - 1; j >= 0 && columns_to_write > 0;
				j--) {
			if (j % 8 == 0) {
				fprintf(stream, " ");
			}

			fprintf(stream, "%d", BIT_CHECK(line[i], j));

			columns_to_write--;
		}

		if (i == 0 && g_n_longs > 2) {
			// Too many to write, let's jump to the end
			i = g_n_longs - 2;
			fprintf(stream, " ... ");
		}
	}

	//fprintf(stream, "\n");
}

/**
 * Prints the whole dataset
 */
void print_dataset(FILE *stream, const char *title, uint_fast64_t *dataset,
		const uint_fast8_t extra_bits) {

	fprintf(stream, "%s\n", title);

	uint_fast64_t *line = dataset;

	for (uint_fast32_t i = 0; i < g_n_observations; i++) {
		print_line(stream, line, extra_bits);
		fprintf(stream, "\n");
		NEXT(line);

		if (g_n_observations > 20 && i == 9) {
			fprintf(stream, " ... \n");
			//skip
			i = g_n_observations - 10;
			line = dataset + (g_n_observations - 10) * g_n_longs;
		}
	}
}

/**
 * Compares two lines of the dataset
 * Used to sort the dataset
 */
int compare_lines(const void *a, const void *b) {

	for (uint_fast32_t i = 0; i < g_n_longs; i++) {

		uint_fast64_t va = *((const uint_fast64_t*) a + i);
		uint_fast64_t vb = *((const uint_fast64_t*) b + i);

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
uint_fast8_t has_same_attributes(const uint_fast64_t *a, const uint_fast64_t *b) {

	uint_fast64_t res = 0;

	for (uint_fast32_t i = 0; i < g_n_longs - 1; i++) {
		res = *(a + i) - *(b + i);
		if (res != 0) {
			return 0;
		}
	}

	uint_fast32_t remaining_attributes = g_n_attributes % BLOCK_BITS;

	if (remaining_attributes == 0) {
		// Nothing more to check
		return 1;
	}

	uint_fast64_t mask = ~0LU;
	mask >>= remaining_attributes;
	mask = ~mask;

	if ((a[g_n_longs - 1] & mask) != (b[g_n_longs - 1] & mask)) {
		return 0;
	}

	return 1;
}

/**
 * Removes duplicated lines from the dataset.
 * Assumes the dataset is ordered
 */
uint_fast32_t remove_duplicates(uint_fast64_t *dataset) {

	uint_fast64_t *line = dataset;
	uint_fast64_t *last = line;

	uint_fast32_t n_uniques = 1;

	for (uint_fast32_t i = 0; i < g_n_observations - 1; i++) {
		NEXT(line);
		if (compare_lines(line, last) != 0) {
			NEXT(last);
			n_uniques++;
			if (last != line) {
				memcpy(last, line, sizeof(uint_fast64_t) * g_n_longs);
			}
		}
	}

	// Update number of unique observations
	g_n_observations = n_uniques;
	return g_n_observations;
}

/**
 * Fill the arrays with the number os items per class and also a matrix with
 * references to the lines that belong to each class to simplify the
 * calculation of the disjoint matrix
 *
 * Inputs are expected to be zeroed arrays
 */
void fill_class_arrays(uint_fast64_t *dataset, uint_fast32_t *n_items_per_class,
		uint_fast64_t **observations_per_class) {

	// Current line
	uint_fast64_t *line = dataset;

	// This line class
	uint_fast8_t clas = 0;

	for (uint_fast32_t i = 0; i < g_n_observations; i++) {
		clas = get_class(line);

		observations_per_class[clas * g_n_observations + n_items_per_class[clas]] =
				line;

		n_items_per_class[clas]++;

		NEXT(line);
	}
}
