/*
 ============================================================================
 Name        : set_cover_hdf5.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to apply the set cover algorithm
 ============================================================================
 */

#include "dataset_hdf5.h"
#include "set_cover.h"
#include "types/cover_t.h"
#include "types/dataset_hdf5_t.h"
#include "types/dm_t.h"
#include "types/oknok_t.h"
#include "types/word_t.h"
#include "utils/bit.h"

#include "hdf5.h"

#include <assert.h>
#include <set_cover_hdf5.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

oknok_t get_column(const hid_t dataset_id, const uint32_t attribute,
				   const uint32_t n_words, word_t* column)
{
	/**
	 * Setup offset
	 */
	hsize_t offset[2] = { attribute, 0 };

	/**
	 * Setup count
	 */
	hsize_t count[2] = { 1, n_words };

	const hsize_t dimensions[2] = { 1, n_words };

	/**
	 * Create a memory dataspace to indicate the size of our buffer/chunk
	 */
	hid_t memspace_id = H5Screate_simple(2, dimensions, NULL);

	/**
	 * Setup line dataspace
	 */
	hid_t dataspace_id = H5Dget_space(dataset_id);

	// Select hyperslab on file dataset
	H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, offset, NULL, count,
						NULL);

	// Read line from dataset
	H5Dread(dataset_id, H5T_NATIVE_UINT64, memspace_id, dataspace_id,
			H5P_DEFAULT, column);

	H5Sclose(dataspace_id);
	H5Sclose(memspace_id);

	return OK;
}

oknok_t update_attribute_totals_add(cover_t* cover,
									dataset_hdf5_t* line_dataset)
{
	oknok_t ret = OK;

	word_t* line = (word_t*) malloc(sizeof(word_t) * cover->n_words_in_a_line);
	assert(line != NULL);

	/**
	 * Define the lines of the physical dataset to process
	 */
	uint32_t current_line = 0;
	// uint32_t end_line	  = cover->n_matrix_lines;

	// Reset totals
	memset(cover->attribute_totals, 0, cover->n_attributes * sizeof(uint32_t));

	for (uint32_t w = 0; w < cover->n_words_in_a_column; w++)
	{
		word_t attribute_values = cover->covered_lines[w];
		/*
				uint8_t last_bit_to_process = 0;
				if (end_line - current_line < WORD_BITS)
				{
					last_bit_to_process = WORD_BITS - (end_line - current_line);
				}
		*/
		// Check the bits
		for (int8_t bit = WORD_BITS - 1;
			 bit >= 0 && current_line < cover->n_matrix_lines;
			 bit--, current_line++)
		{
			if (!BIT_CHECK(attribute_values, bit))
			{
				// This line is not covered

				// Read line from dataset
				hdf5_read_line(line_dataset, current_line,
							   cover->n_words_in_a_line, line);

				// Increment totals
				add_line_contribution(cover, line);
			}
		}
	}

	free(line);

	return ret;
}

oknok_t update_attribute_totals_sub(cover_t* cover,
									dataset_hdf5_t* line_dataset,
									word_t* column)
{
	oknok_t ret = OK;

	word_t* line = (word_t*) malloc(sizeof(word_t) * cover->n_words_in_a_line);
	assert(line != NULL);

	/**
	 * Define the lines of the physical dataset to process
	 */
	uint32_t current_line = 0;

	for (uint32_t w = 0; w < cover->n_words_in_a_column; w++)
	{
		// cov col
		//   0   0   0
		//   0   1   1
		//   1   0   0
		//   1   1   0

		word_t attribute_values = ~cover->covered_lines[w] & column[w];

		// Check the bits
		for (int8_t bit = WORD_BITS - 1;
			 bit >= 0 && current_line < cover->n_matrix_lines;
			 bit--, current_line++)
		{
			if (BIT_CHECK(attribute_values, bit))
			{
				// This line is covered

				// Read line from dataset
				hdf5_read_line(line_dataset, current_line,
							   cover->n_words_in_a_line, line);

				// Increment totals
				sub_line_contribution(cover, line);
			}
		}
	}

	free(line);

	return ret;
}
