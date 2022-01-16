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
unsigned int get_class(const unsigned long *line) {

	// Check how many attributes remain on last long
	int remaining_attributes = g_n_attributes % LONG_BITS;

	// Class starts here
	int at = LONG_BITS - remaining_attributes - g_n_bits_for_class;

	return (unsigned int) get_bits(line[g_n_longs - 1], at, g_n_bits_for_class);
}

/**
 * Prints a line to stream
 */
void print_line(FILE *stream, const unsigned long *line, const char extra_bits) {

	// Current attribute
	unsigned long columns_to_write = g_n_attributes;

	if (extra_bits == 1) {
		columns_to_write += g_n_bits_for_class;
	}

	for (unsigned int i = 0; i < g_n_longs && columns_to_write > 0; i++) {
		for (int j = LONG_BITS - 1; j >= 0 && columns_to_write > 0; j--) {
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
void print_dataset(FILE *stream, const char *title, unsigned long *dataset,
		const char extra_bits) {

	fprintf(stream, "%s\n", title);

	unsigned long *line = dataset;
	for (unsigned long i = 0; i < g_n_observations; i++) {
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

	long long res = 0;

	for (unsigned int i = 0; i < g_n_longs; i++) {
		res = *((unsigned long*) a + i) - *((unsigned long*) b + i);
		if (res > 0) {
			return 1;
		}
		if (res < 0) {
			return -1;
		}
	}
	return 0;
}

/**
 * Checks if the lines have the same attributes
 */
int has_same_attributes(const unsigned long *a, const unsigned long *b) {

	long long res = 0;

	for (unsigned int i = 0; i < g_n_longs - 1; i++) {
		res = *(a + i) - *(b + i);
		if (res != 0) {
			return 0;
		}
	}

	unsigned int remaining_attributes = g_n_attributes % LONG_BITS;

	if (remaining_attributes == 0) {
		// Nothing more to check
		return 1;
	}

	unsigned long mask = ~0L;
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
unsigned long remove_duplicates(unsigned long *dataset) {

	unsigned long *line = dataset;
	unsigned long *last = line;

	unsigned long n_uniques = 1;

	for (unsigned long i = 0; i < g_n_observations - 1; i++) {
		NEXT(line);
		if (compare_lines(line, last) != 0) {
			NEXT(last);
			n_uniques++;
			if (last != line) {
				memcpy(last, line, sizeof(unsigned long) * g_n_longs);
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
void fill_class_arrays(unsigned long *dataset, unsigned long *n_items_per_class,
		unsigned long **observations_per_class) {

	// Current line
	unsigned long *line = dataset;

	// This line class
	unsigned int clas = 0;

	for (unsigned long i = 0; i < g_n_observations; i++) {
		clas = get_class(line);

		observations_per_class[clas * g_n_observations + n_items_per_class[clas]] =
				line;

		n_items_per_class[clas]++;

		NEXT(line);
	}
}
