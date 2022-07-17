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
 * Calculates the number of lines for the disjoint matrix
 */
uint32_t calculate_number_of_lines_of_disjoint_matrix(const dataset_t *dataset);

/**
 * Checks if the disjoint matrix datasets are already present in the hdf5 file
 */
bool is_matrix_created(const char *filename);

/**
 * Builds the disjoint matrix and saves it to the hdf5 dataset file
 * It will build and store 2 datasets one with attributes as lines
 * the other with atributes as columns
 */
oknok_t create_disjoint_matrix(const char *filename, const dataset_t *dataset);

/**
 * Creates the disjoint matrix datasets
 */
oknok_t create_disjoint_matrix_datasets(const hid_t file_id,
		const dataset_t *dataset);

/**
 * Creates the dataset containing the disjoint matrix with attributes as columns
 */
oknok_t create_line_dataset(const hid_t file_id, const dataset_t *dataset);

/**
 * Creates the dataset containing the disjoint matrix with attributes as lines
 */
oknok_t create_column_dataset(const hid_t file_id, const dataset_t *dataset);

/**
 * Writes the matrix atributes in the dataset
 */
herr_t write_disjoint_matrix_attributes(const hid_t dataset_id,
		const uint32_t n_attributes, const uint32_t n_matrix_lines);

herr_t save_attribute_data(const hid_t dm_dataset_id,
		const hid_t dm_dataset_space_id, const hid_t dm_memory_space_id,
		hsize_t *offset, const hsize_t *count, const word_t *data,
		const uint32_t n_lines, const uint8_t n_attributes);

/**
 * Writes the line totals metadata to the dataset
 */
herr_t write_line_totals_data(const hid_t file_id, const uint32_t *data,
		const uint32_t n_lines);

#endif
