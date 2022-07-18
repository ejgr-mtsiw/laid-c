/*
 ============================================================================
 Name        : set_cover.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to apply the set cover algorithm
 ============================================================================
 */

#ifndef SET_COVER_H
#define SET_COVER_H

#include "bit_utils.h"
#include "dataset.h"
#include "globals.h"
#include "hdf5.h"
#include "hdf5_dataset.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct cover_t {

	/**
	 * Number of attributes
	 */
	uint32_t n_attributes;

	/**
	 * Number of lines of the disjoint matrix
	 */
	uint32_t n_matrix_lines;

	/**
	 * Array of line totals
	 */
	uint32_t *line_totals;

	/**
	 * Bit array of lines to cover
	 */
	word_t *uncovered_lines;

	/**
	 * Array of selected attributes
	 */
	uint8_t *selected_attributes;

} cover_t;

/**
 * Applies the set cover algorithm using the hdf5 datasets
 */
oknok_t calculate_solution(const char *filename, cover_t *cover);

/**
 * Reads a line from the dataset
 */
oknok_t hdf5_read_line(const hid_t dataset_id, const hid_t dataspace_id,
		const hid_t memspace_id, uint32_t index, const uint32_t n_words,
		word_t *line);

/**
 * Calculate index of best line using cover->line_totals
 * and cover->uncovered_lines
 *
 * @Returns index of best line or -1 if all are covered
 */
int64_t get_best_line_index(const cover_t *cover);

/**
 * Reads the disjoint matrix dataset and blacklists the lines that
 * depend on the attribute to blacklist
 */
herr_t blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const cover_t *cover,
		const uint32_t attribute_to_blacklist);

/**
 * Calculates the initial totals for all attributes
 */
void calculate_initial_sum(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const cover_t *cover);

/**
 * Prints the attributes that are part of the solution
 */
void print_solution(FILE *stream, cover_t *cover);

/**
 * Frees the allocated resources
 */
void free_cover(cover_t *cover);
#endif

