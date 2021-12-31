/*
 ============================================================================
 Name        : dataset_line.h
 Author      : Eduardo Ribeiro
 Description : Structure to store info about one line of the dataset
 ============================================================================
 */

#ifndef DATASET_LINE_H
#define DATASET_LINE_H

#include "bit_utils.h"
#include <stdio.h>
#include <string.h>

typedef struct dataset_line {
	/**
	 * The class of this line
	 */
	unsigned int class_id;

	/**
	 * Inconsistency rating for this line
	 * How many entries have the same attributes but different classes?
	 * 0 means no inconsistency
	 */
	unsigned int inconsistency;

	/**
	 * Pointer to start address of attributes in full dataset
	 */
	unsigned long *data;
} dataset_line;

/**
 * Extracts the class value from a line read from the hdf5 dataset
 */
unsigned int get_class(const unsigned long *buffer, const unsigned long n_attributes, unsigned int n_bits_for_classes);

/**
 * Copies one dataset_line info to another
 */
void dataset_line_copy(dataset_line *to, const dataset_line *from, const unsigned long n_attributes);

/**
 * Replaces the class bits with jnsq bits
 * It's ok, because the number of jnsq bits is always <= bits needed for class
 * And we don't need the class anymore because we extrtacted it to the
 * dataset_line structure
 */
void set_jnsq(dataset_line *new_line, const unsigned long n_attributes, const unsigned int bits_for_jnsq);

/**
 * Checks if the dataset lines have the same attributes
 */
int has_same_attributes(const dataset_line *a, const dataset_line *b, const unsigned long n_attributes);

/**
 * Checks if the dataset lines have the same class
 */
int has_same_class(const dataset_line *a, const dataset_line *b);

/**
 * Prints a line to stdout
 */
void print_line(const dataset_line *line, const unsigned long n_attributes);

#endif
