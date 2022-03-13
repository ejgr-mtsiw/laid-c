/*
 ============================================================================
 Name        : disjoint_matrix.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage the disjoint matrix
 ============================================================================
 */

#ifndef DISJOINT_MATRIX_H
#define DISJOINT_MATRIX_H

#include "globals.h"
#include "hdf5.h"
#include <malloc.h>
#include <stdint.h>

/**
 * The name of the dataset that will store the disjoint matrix
 */
#define DISJOINT_MATRIX_DATASET_NAME "DMX"

/**
 * Calculates the number of lines for the disjoint matrix
 */
uint_fast64_t calculate_number_of_lines(const uint_fast32_t *n_items_per_class);

/**
 * Builds the disjoint matrix and saves it to the hdf5 dataset file
 */
herr_t create_disjoint_matrix(const char *filename, const char *datasetname,
		const uint_fast32_t *n_items_per_class,
		uint_fast32_t **observations_per_class);

/**
 * Creates a new disjoint matrix dataset
 */
status_t create_new_disjoint_matrix_dataset(hid_t file_id,
		const uint_fast32_t *n_items_per_class,
		uint_fast32_t **observations_per_class);

#endif
