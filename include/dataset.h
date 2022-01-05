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
#include "hdf5.h"
#include <malloc.h>
#include <math.h>
#include <string.h>

#define OK 0

#define DATASET_VALID_DIMENSIONS 0
#define DATASET_INVALID_DIMENSIONS 1

#define ERROR_ALLOCATING_DATASET_DATA 1

/**
 * Number of ranks for data
 */
#define DATA_RANK 2

/**
 *
 */

#define NEXT(line) ((line) += (n_longs))

/**
 *
 */

/**
 * Dataset dimensions
 */
extern hsize_t dimensions[2];

/**
 * Number of attributes
 */
extern unsigned long n_attributes;

/**
 * Number of observations
 */
extern unsigned long n_observations;

/**
 * Number of classes
 */
extern unsigned int n_classes;

/**
 * Number of bits used to store the class
 */
extern unsigned int n_bits_for_class;

/**
 * Number of longs needed to store one line of the dataset
 */
extern unsigned int n_longs;

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
void print_line(FILE *stream, const unsigned long *line);

/**
 * Prints the whole dataset
 */
void print_dataset(FILE *stream, unsigned long *dataset, const char *title);

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
 * Replaces the class bits with jnsq bits
 * It's ok, because the number of jnsq bits is always <= bits needed for class
 * And we don't need the class anymore because we extracted it to the
 * dataset_line structure
 */
void set_jnsq(unsigned long *line, unsigned long inconsistency);

/**
 * Adds the JNSQs attributes to the dataset
 */
unsigned long add_jnsqs(unsigned long *dataset);

/**
 * Removes duplicated lines from the dataset.
 * Assumes the dataset is ordered
 */
int remove_duplicates(unsigned long *dataset);

/**
 * Fill the arrays with the number os items per class and also a matrix with
 * references to the lines that belong to eacv class to simplify the
 * calculation of the disjoint matrix
 */
void fill_class_arrays(unsigned long *dataset, unsigned long *n_items_per_class,
		unsigned long **observations_per_class);

#endif
