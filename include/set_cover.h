/*
 ============================================================================
 Name        : set_cover.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to apply the set cover algorithm
 ============================================================================
 */

#ifndef SET_COVER_H
#define SET_COVER_H

#include "cost.h"
#include "disjoint_matrix.h"
#include "globals.h"
#include "hdf5.h"
#include <string.h>

/**
 * Applies the set cover algorithm to the hdf5 dataset and prints
 * the minimum attribute set that covers all the lines
 */
int calculate_solution(const char *filename, const char *datasetname,
		const unsigned long *n_items_per_class);

int blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const unsigned long n_lines,
		const unsigned long attribute_to_blacklist,
		unsigned char *line_blacklist);

#endif
