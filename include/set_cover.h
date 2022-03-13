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
#include "disjoint_matrix.h"
#include "globals.h"
#include "hdf5.h"
#include <stdint.h>
#include <string.h>

/**
 * Applies the set cover algorithm to the hdf5 dataset and prints
 * the minimum attribute set that covers all the lines
 */
status_t calculate_solution(const char *filename, const char *datasetname,
		const uint_fast32_t *n_items_per_class);

status_t blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const uint_fast32_t n_lines,
		const uint_fast32_t attribute_to_blacklist,
		uint_fast8_t *line_blacklist);

/**
 * Finds the next attribute to blacklist
 */
uint_fast32_t update_sum(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const uint_fast32_t n_lines,
		const uint_fast8_t *line_blacklist,
		const uint_fast8_t *attribute_blacklist, uint_fast32_t *sum);

/**
 * Releases resources handles
 */
void close_resources(hid_t file_id, hid_t dataset_id, hid_t dm_dataset_space_id,
		hid_t dm_memory_space_id);

#endif
