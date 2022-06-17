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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

//
#define NOT_BLACKLISTED false
#define BLACKLISTED true

#define DATASET_OK 0
#define DATASET_INVALID_DIMENSIONS 1
#define DATASET_NOT_ENOUGH_CLASSES 2
#define DATASET_NOT_ENOUGH_ATTRIBUTES 4
#define DATASET_NOT_ENOUGH_OBSERVATIONS 8
#define DATASET_ERROR_ALLOCATING_DATA 16

/**
 *
 */

#define NEXT_LINE(line, n_longs) ((line) += (n_longs))

//#define GET_NEXT_OBSERVATION(line, n_longs) ((line) + (n_longs))
//#define GET_PREV_OBSERVATION(line, n_longs) ((line) - (n_longs))

#define GET_LAST_OBSERVATION(dset, n_observations, n_longs) ((dset) + ((n_observations - 1) * n_longs))

typedef struct dataset_t {
	/**
	 * Number of attributes
	 */
	unsigned int n_attributes;

	/**
	 * Number of longs (64bits) needed to store a line
	 */
	unsigned int n_longs;

	/**
	 * Number of bits needed to store jnsqs
	 */
	unsigned int n_bits_for_jnsqs;

	/**
	 * Number of observations
	 */
	unsigned int n_observations;

	/**
	 * Number of classes
	 */
	unsigned int n_classes;

	/**
	 * Number of bits used to store the class
	 */
	unsigned int n_bits_for_class;

	/**
	 * Dataset data
	 */
	unsigned long *data;

	/**
	 * Array with number of observations per class
	 */
	unsigned int *n_observations_per_class;

	/**
	 * Array with pointers for each observation per class.
	 * They reference lines in *data
	 */
	unsigned long **observations_per_class;

} dataset_t;

/**
 * Returns the class of this data line
 */
unsigned int get_class(const unsigned long *line,
		const unsigned int n_attributes, const unsigned int n_longs,
		const unsigned int n_bits_for_class);

/**
 * Compares two lines of the dataset
 * Used to sort the dataset
 */
//int compare_lines(const void *a, const void *b);
int compare_lines_extra(const void *a, const void *b, void *n_longs);

/**
 * Checks if the lines have the same attributes
 */
bool has_same_attributes(const unsigned long *a, const unsigned long *b,
		const unsigned int n_attributes, const unsigned long n_longs);

/**
 * Removes duplicated lines from the dataset.
 * Assumes the dataset is ordered
 * Returns number of removed observations
 */
unsigned int remove_duplicates(dataset_t *dataset);

/**
 * Fill the arrays with the number of items per class and also a matrix with
 * references to the lines that belong to each class to simplify the
 * calculation of the disjoint matrix
 */
void fill_class_arrays(dataset_t *dataset);

#endif
