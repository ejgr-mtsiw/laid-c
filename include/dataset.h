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
#include <stdint.h>
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
uint_fast8_t get_class(const uint_fast64_t *line);

/**
 * Prints a line to stream
 */
void print_line(FILE *stream, const uint_fast64_t *line,
		const uint_fast8_t extra_bits);

/**
 * Prints the whole dataset
 */
void print_dataset(FILE *stream, const char *title, uint_fast64_t *dataset,
		const uint_fast8_t extra_bits);

/**
 * Compares two lines of the dataset
 * Used to sort the dataset
 */
int compare_lines(const void *a, const void *b);

/**
 * Checks if the lines have the same attributes
 */
uint_fast8_t has_same_attributes(const uint_fast64_t *a, const uint_fast64_t *b);

/**
 * Removes duplicated lines from the dataset.
 * Assumes the dataset is ordered
 */
uint_fast32_t remove_duplicates(uint_fast64_t *dataset);

/**
 * Fill the arrays with the number os items per class and also a matrix with
 * references to the lines that belong to eacv class to simplify the
 * calculation of the disjoint matrix
 */
void fill_class_arrays(uint_fast64_t *dataset, uint_fast32_t *n_items_per_class,
		uint_fast64_t **observations_per_class);

#endif
