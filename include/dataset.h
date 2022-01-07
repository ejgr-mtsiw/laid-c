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

#define OK 0

#define WITHOUT_EXTRA_BITS 0
#define WITH_EXTRA_BITS 1

#define DATASET_INVALID_DIMENSIONS 1
#define DATASET_NOT_ENOUGH_CLASSES 2
#define DATASET_NOT_ENOUGH_ATTRIBUTES 3
#define DATASET_NOT_ENOUGH_OBSERVATIONS 4

#define ERROR_ALLOCATING_DATASET_DATA 1

/**
 * Number of ranks for data
 */
#define DATA_RANK 2

/**
 *
 */

#define NEXT(line) ((line) += (g_n_longs))

#define GET_NEXT_OBSERVATION(line) ((line) + (g_n_longs))
#define GET_PREV_OBSERVATION(line) ((line) - (g_n_longs))

#define GET_LAST_OBSERVATION(dset) ((dset) + (g_n_observations * g_n_longs))

/**
 *
 */

/**
 * Reads the dataset attributes and fills the global variables
 */
int read_attributes(const hid_t dataset_id);

/**
 * Reads the value of one attribute from the dataset
 */
herr_t read_attribute(hid_t dataset_id, const char *attribute, hid_t datatype,
		void *value);

/**
 * Reads chunk dimensions from dataset if chunking was enabled
 */
int get_chunk_dimensions(const hid_t dataset_id, hsize_t *chunk_dimensions);

/**
 * Returns the dataset dimensions stored in the hdf5 dataset
 */
void get_dataset_dimensions(hid_t dataset_id, hsize_t *dataset_dimensions);

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
