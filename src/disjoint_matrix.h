/*
 ============================================================================
 Name        : disjoint_matrix.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage the disjoint matrix
 ============================================================================
 */

#ifndef DISJOINT_MATRIX_H
#define DISJOINT_MATRIX_H

#include "types/dataset_hdf5_t.h"
#include "types/dataset_t.h"
#include "types/dm_t.h"
#include "types/oknok_t.h"

#include "hdf5.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * Number of lines to buffer before output
 * Only used in the line dataset, because we already write WORD_BITS columns at
 * a time in the column dataset
 */
#define N_LINES_OUT 42

/**
 * Calculates the number of lines for the disjoint matrix
 */
uint32_t get_dm_n_lines(const dataset_t* dataset);

/**
 * Builds one column of the disjoint matriz
 * One column represents WORD_BITS attributes. It's equivalent to reading
 * the first word from every line from the line disjoint matrix
 */
oknok_t generate_dm_column(const dm_t* dm, const int column, word_t* buffer);

/**
 * Writes the matrix atributes in the dataset
 */
herr_t write_dm_attributes(const hid_t dataset_id, const uint32_t n_attributes,
						   const uint32_t n_matrix_lines);

/**
 * Generates the steps for the partial disjoint matrix dm
 */
oknok_t generate_steps(const dataset_t* dataset, dm_t* dm);

/**
 * Creates the dataset containing the disjoint matrix with attributes as columns
 */
oknok_t create_line_dataset(const dataset_hdf5_t* hdf5_dset,
							const dataset_t* dset, const dm_t* dm);

/**
 * Creates the dataset containing the disjoint matrix with attributes as
 * lines
 */
oknok_t create_column_dataset(const dataset_hdf5_t* hdf5_dset,
							  const dataset_t* dset, const dm_t* dm);

/**
 * Writes the attribute totals metadata to the dataset
 */
oknok_t write_attribute_totals(const hid_t dataset_id,
							   const uint32_t n_attributes,
							   const uint32_t* data);

#endif
