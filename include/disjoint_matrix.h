/*
 ============================================================================
 Name        : disjoint_matrix.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage the disjoint matrix
 ============================================================================
 */

#ifndef DISJOINT_MATRIX_H
#define DISJOINT_MATRIX_H

#include "dataset.h"
#include "dataset_hdf5.h"
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
unsigned long calculate_number_of_lines_of_disjoint_matrix(
		const dataset_t *dataset);

/**
 * Builds the disjoint matrix and saves it to the hdf5 dataset file
 */
herr_t create_disjoint_matrix(const char *filename, const dataset_t *dataset);

/**
 * Creates a new disjoint matrix dataset
 */
herr_t create_disjoint_matrix_dataset(const hid_t file_id,
		const dataset_t *dataset);

#endif
