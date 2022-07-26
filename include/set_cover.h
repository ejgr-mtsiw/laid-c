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
	 * Number of words needed to store a line
	 */
	uint32_t n_words;

	/**
	 * Number of lines of the disjoint matrix
	 */
	uint32_t matrix_n_lines;

	/**
	 * Array of bllacklisted lines
	 */
	uint8_t *line_blacklist;

	/**
	 * Array of blacklisted attributes
	 */
	uint8_t *attribute_blacklist;

	/**
	 * Array with the current totals for all attributes
	 */
	uint32_t *sum;

} cover_t;

/**
 * Applies the set cover algorithm to the hdf5 dataset and prints
 * the minimum attribute set that covers all the lines
 */
oknok_t calculate_solution(const char *filename, cover_t *cover);

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
void calculate_initial_sum(const char *filename, const cover_t *cover);

/**
 * Prints the attributes that are part of the solution
 */
void print_solution(FILE *stream, cover_t *cover);

/**
 * Frees the allocated resources
 */
void free_cover(cover_t *cover);
#endif

