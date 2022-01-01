/*
 ============================================================================
 Name        : dataset_lines.h
 Author      : Eduardo Ribeiro
 Description : Structure to store info about the lines of the dataset
 ============================================================================
 */

#ifndef DATASET_LINES_H
#define DATASET_LINES_H

#include "bit_utils.h"
#include "dataset_line.h"
#include <malloc.h>

typedef struct dataset_lines {
	/**
	 * Number of lines / observations
	 */
	unsigned long n_lines;

	/**
	 * Lines of the dataset
	 */
	dataset_line *lines;

	/**
	 * The first line
	 */
	dataset_line *first;

	/**
	 * The last line
	 */
	dataset_line *last;
} dataset_lines;

/**
 * Prepares the dataset_lines structure
 */
void setup_dataset_lines(dataset_lines *dataset_lines, unsigned long n_observations);

/**
 * Checks if a line already is present and if true if it is a duplicate
 * or an inconsistent one (and updates line inconsistency)
 */
int check_duplicate_inconsistent(const dataset_lines *dataset_lines, dataset_line *new_line,
		const unsigned long n_attributes);

/**
 * Adds a line to the dataset
 */
void add_line(dataset_lines *dataset_lines, const dataset_line *new_line);

#endif
