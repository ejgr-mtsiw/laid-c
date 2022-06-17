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
#include "disjoint_matrix.h"
#include "globals.h"
#include "hdf5.h"
#include <stdint.h>
#include <string.h>

typedef struct cover_t {

	/**
	 * Number of attributes
	 */
	unsigned int n_attributes;

	/**
	 * Number of longs (64bits) needed to store a line
	 */
	unsigned int n_longs;

	/**
	 * Number of lines of the disjoint matrix
	 */
	unsigned long matrix_n_lines;

	/**
	 * Array of bllacklisted lines
	 */
	unsigned char *line_blacklist;

	/**
	 * Array of blacklisted attributes
	 */
	unsigned char *attribute_blacklist;

	/**
	 * Array with the current totals for all attributes
	 */
	unsigned int *sum;

} cover_t;

/**
 * Applies the set cover algorithm to the hdf5 dataset and prints
 * the minimum attribute set that covers all the lines
 */
herr_t calculate_solution(const char *filename, const char *datasetname);

/**
 * Reads the disjoint matrix dataset and blacklists the lines that
 * depend on the attribute to blacklist
 */
herr_t blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const cover_t *cover,
		const unsigned int attribute_to_blacklist);

/**
 * Calculates the initial totals for all attributes
 */
unsigned int calculate_initial_sum(const hid_t dataset_id,
		const hid_t dataset_space_id, const hid_t memory_space_id,
		const cover_t *cover);

//void update_sum(const unsigned long *buffer,
//		const unsigned char *attribute_blacklist);

/**
 * Releases resources handles
 */
void close_resources(hid_t file_id, hid_t dataset_id, hid_t dm_dataset_space_id,
		hid_t dm_memory_space_id);

#endif
