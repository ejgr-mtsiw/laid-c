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
#include "globals.h"
#include "hdf5.h"
#include "hdf5_dataset.h"
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * The name of the dataset that will store the disjoint matrix
 * with attributes as lines
 */
#define DM_DATASET_ATTRIBUTES_LINE "DMX_LINE"

/**
 * The name of the dataset that will store the disjoint matrix
 * with attributes as columns
 */
#define DM_DATASET_ATTRIBUTES_COLUMN "DMX_COLUMN"

/**
 * Calculates the number of lines for the disjoint matrix
 */
unsigned long calculate_number_of_lines_of_disjoint_matrix(
		const dataset_t *dataset);

/**
 * Checks if the disjoint matrix datasets are already present in the hdf5 file
 */
bool is_matrix_created(const char *filename);

/**
 * Builds the disjoint matrix and saves it to the hdf5 dataset file
 * It will build and store 2 datasets one with attributes as lines
 * the other with atributes as columns
 */
int create_disjoint_matrix(const char *filename, const dataset_t *dataset);

/**
 * Creates the disjoint matrix datasets
 */
int create_disjoint_matrix_datasets(const hid_t file_id,
		const dataset_t *dataset);

/**
 * Creates the dataset containing the disjoint matrix with attributes as columns
 */
int create_attribute_column_dataset(const hid_t file_id,
		const dataset_t *dataset);

/**
 * Creates the dataset containing the disjoint matrix with attributes as lines
 */
int create_attribute_line_dataset(const hid_t file_id, const dataset_t *dataset);

/**
 * Writes the matrix atributes in the dataset
 */
herr_t write_disjoint_matrix_attributes(const hid_t dataset_id,
		const unsigned int n_attributes, const unsigned int n_matrix_lines);

herr_t save_attribute_data(const hid_t dm_dataset_id,
		const hid_t dm_dataset_space_id, const hid_t dm_memory_space_id,
		hsize_t *offset, const hsize_t *count, const unsigned long *data,
		const unsigned int n_lines, const int n_attributes);

#endif
