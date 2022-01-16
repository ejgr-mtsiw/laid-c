/*
 ============================================================================
 Name        : dataset.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage datasets
 ============================================================================
 */

#ifndef DATASET_H
#define DATASET_H

#include "bit_utils.h"
#include "globals.h"
#include "hdf5.h"
#include <math.h>
#include <string.h>

// Used in debugging
#ifdef DEBUG
#define DEBUG_PRINT_DATASET(stream, title,dataset, extra) print_dataset(stream, title, dataset, extra);
#else
#define DEBUG_PRINT_DATASET(stream, title,dataset, extra)
#endif

#define WITHOUT_EXTRA_BITS 0
#define WITH_EXTRA_BITS 1

/**
 *
 */

#define NEXT(line) ((line) += (g_n_longs))

#define GET_NEXT_OBSERVATION(line) ((line) + (g_n_longs))
#define GET_PREV_OBSERVATION(line) ((line) - (g_n_longs))

#define GET_LAST_OBSERVATION(dset) ((dset) + ((g_n_observations - 1) * g_n_longs))

/**
 * Returns the class of this data line
 */
unsigned int get_class(const unsigned long *line);

/**
 * Prints a line to stream
 */
void print_line(FILE *stream, const unsigned long *line, const char extra_bits);

/**
 * Prints the whole dataset
 */
void print_dataset(FILE *stream, const char *title, unsigned long *dataset,
		const char extra_bits);

/**
 * Compares two lines of the dataset
 * Used to sort the dataset
 */
int compare_lines(const void *a, const void *b);

/**
 * Checks if the lines have the same attributes
 */
int has_same_attributes(const unsigned long *a, const unsigned long *b);

/**
 * Removes duplicated lines from the dataset.
 * Assumes the dataset is ordered
 */
unsigned long remove_duplicates(unsigned long *dataset);

/**
 * Fill the arrays with the number os items per class and also a matrix with
 * references to the lines that belong to eacv class to simplify the
 * calculation of the disjoint matrix
 */
void fill_class_arrays(unsigned long *dataset, unsigned long *n_items_per_class,
		unsigned long **observations_per_class);

#endif
