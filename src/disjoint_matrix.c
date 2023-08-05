/*
 ============================================================================
 Name        : disjoint_matrix.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage the disjoint matrix
 ============================================================================
 */

#include "disjoint_matrix.h"

#include "dataset_hdf5.h"
#include "types/dataset_hdf5_t.h"
#include "types/dataset_t.h"
#include "types/dm_t.h"
#include "types/oknok_t.h"
#include "types/word_t.h"
#include "utils/bit.h"
#include "utils/timing.h"

#include "hdf5.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint32_t get_dm_n_lines(const dataset_t* dataset)
{
	uint32_t n = 0;

	uint32_t n_classes	  = dataset->n_classes;
	uint32_t* n_class_obs = dataset->n_observations_per_class;

	for (uint32_t i = 0; i < n_classes - 1; i++)
	{
		for (uint32_t j = i + 1; j < n_classes; j++)
		{
			n += n_class_obs[i] * n_class_obs[j];
		}
	}

	return n;
}

oknok_t generate_dm_column(const dm_t* dm, const int column, word_t* buffer)
{
	// Current buffer line
	word_t* bl = buffer;

	for (uint32_t cl = 0; cl < dm->n_matrix_lines; cl++)
	{
		word_t* la = dm->steps[cl].lineA;
		word_t* lb = dm->steps[cl].lineB;

		(*bl) = la[column] ^ lb[column];
		bl++;
	}

	return OK;
}

herr_t write_dm_attributes(const hid_t dataset_id, const uint32_t n_attributes,
						   const uint32_t n_matrix_lines)
{
	herr_t ret = 0;

	ret = hdf5_write_attribute(dataset_id, N_ATTRIBUTES_ATTR, H5T_NATIVE_UINT,
							   &n_attributes);
	if (ret < 0)
	{
		return ret;
	}

	ret = hdf5_write_attribute(dataset_id, N_MATRIX_LINES_ATTR, H5T_NATIVE_UINT,
							   &n_matrix_lines);

	return ret;
}

oknok_t generate_steps(const dataset_t* dataset, dm_t* dm)
{

	uint32_t nc	   = dataset->n_classes;
	uint32_t nobs  = dataset->n_observations;
	word_t** opc   = dataset->observations_per_class;
	uint32_t* nopc = dataset->n_observations_per_class;

	// TODO: I think we can optimize the order of the lines
	// to optimize cache usage and get faster results
	// when calculating the attributes totals

	uint32_t cs = 0;

	// TODO: is there a better way?

	for (uint32_t ca = 0; ca < nc - 1; ca++)
	{
		for (uint32_t ia = 0; ia < nopc[ca]; ia++)
		{
			for (uint32_t cb = ca + 1; cb < nc; cb++)
			{
				for (uint32_t ib = 0; ib < nopc[cb]; ib++)
				{
					// Generate next step
					word_t** bla = opc + ca * nobs;
					word_t** blb = opc + cb * nobs;

					dm->steps[cs].lineA = *(bla + ia);
					dm->steps[cs].lineB = *(blb + ib);
					cs++;
				}
			}
		}
	}

	return OK;
}

oknok_t create_line_dataset(const dataset_hdf5_t* hdf5_dset,
							const dataset_t* dset, const dm_t* dm)
{
	/**
	 * Create line dataset
	 */
	hid_t dset_id = hdf5_create_dataset(hdf5_dset->file_id, DM_LINE_DATA,
										dm->n_matrix_lines, dset->n_words,
										H5T_NATIVE_UINT64);

	// Write dataset attributes
	herr_t err
		= write_dm_attributes(dset_id, dset->n_attributes, dm->n_matrix_lines);
	assert(err != NOK);

	// Allocate output buffer
	word_t* buffer
		= (word_t*) malloc(N_LINES_OUT * dset->n_words * sizeof(word_t));
	assert(buffer != NULL);

	// Current output line index
	uint32_t offset = 0;

	for (uint32_t cl = 0; cl < dm->n_matrix_lines; cl += N_LINES_OUT)
	{
		for (uint32_t w = 0; w < dset->n_words; w += 8)
		{
			for (uint32_t cll = cl;
				 cll < cl + N_LINES_OUT && cll < dm->n_matrix_lines; cll++)
			{
				for (uint32_t ww = w; ww < w + 8 && ww < dset->n_words; ww++)
				{

					word_t* la = dm->steps[cll].lineA;
					word_t* lb = dm->steps[cll].lineB;

					buffer[(cll - cl) * dset->n_words + ww] = la[ww] ^ lb[ww];
				}
			}
		}

		// Number of lines to write
		uint32_t n_lines_out = N_LINES_OUT;
		if (cl + N_LINES_OUT > dm->n_matrix_lines)
		{
			n_lines_out = dm->n_matrix_lines - cl;
		}

		hdf5_write_n_lines(dset_id, offset, n_lines_out, dset->n_words,
						   H5T_NATIVE_UINT64, buffer);

		offset += n_lines_out;
	}

	free(buffer);

	H5Dclose(dset_id);

	return OK;
}

oknok_t create_column_dataset(const dataset_hdf5_t* hdf5_dset,
							  const dataset_t* dset, const dm_t* dm)
{

	// Number of words in a line ON OUTPUT DATASET
	uint32_t out_n_words = dm->n_matrix_lines / WORD_BITS
		+ (dm->n_matrix_lines % WORD_BITS != 0);

	/**
	 * CREATE OUTPUT DATASET
	 */

	hid_t dset_id = hdf5_create_dataset(hdf5_dset->file_id, DM_COLUMN_DATA,
										dset->n_attributes, out_n_words,
										H5T_NATIVE_UINT64);

	/**
	 * Create dataset to hold the totals
	 */
	hid_t totals_dset_id
		= hdf5_create_dataset(hdf5_dset->file_id, DM_ATTRIBUTE_TOTALS, 1,
							  dset->n_attributes, H5T_NATIVE_UINT32);

	/**
	 * Attribute totals buffer
	 */
	uint32_t* attr_buffer = NULL;

	// uint32_t n_words_to_process = dset->n_words;

	// The attribute blocks to generate/save start at
	// uint32_t attribute_block_start = 0;
	// uint32_t attribute_block_end   = n_words_to_process;

	/**
	 * Allocate input buffer
	 *
	 * Input buffer will hold one word per disjoint matrix line
	 * Rounding to nearest multiple of 64 so we don't have to worry when
	 * transposing the last lines
	 */
	word_t* in_buffer = (word_t*) malloc(out_n_words * 64 * sizeof(word_t));

	/**
	 * Allocate output buffer
	 *
	 * Will hold up the matrix lines of up to 64 attributes
	 */
	word_t* out_buffer = (word_t*) malloc(out_n_words * 64 * sizeof(word_t));

	// Start of the lines block to transpose
	word_t* transpose_index = NULL;

	/**
	 * Allocate attribute totals buffer
	 * We save the totals for each attribute.
	 * This saves time when selecting the first best attribute
	 */
	attr_buffer = (uint32_t*) calloc(dset->n_attributes, sizeof(uint32_t));

	uint32_t n_remaining_lines_to_write = dset->n_attributes;

	uint32_t n_lines_to_write = WORD_BITS;

	for (uint32_t current_attribute_word = 0;
		 current_attribute_word < dset->n_words;
		 current_attribute_word++, n_remaining_lines_to_write -= 64)
	{

		if (n_remaining_lines_to_write < 64)
		{
			n_lines_to_write = n_remaining_lines_to_write;
		}

		/**
		 * !TODO: Confirm that generating the column is faster than
		 * reading it from the line dataset
		 */
		generate_dm_column(dm, current_attribute_word, in_buffer);

		transpose_index = in_buffer;

		/**
		 * Transpose all lines but the last one
		 * The last word may not may not have 64 attributes
		 * and we're working with garbage, but it's fine because
		 * we trim them after the main loop
		 */
		uint32_t ow = 0;
		for (ow = 0; ow < out_n_words - 1; ow++, transpose_index += 64)
		{
			// Transpose a 64x64 block in place
			transpose64(transpose_index);

			// Append to output buffer
			for (uint8_t l = 0; l < n_lines_to_write; l++)
			{
				out_buffer[l * out_n_words + ow] = transpose_index[l];

				// Update attribute totals
				attr_buffer[current_attribute_word * WORD_BITS + l]
					+= __builtin_popcountl(transpose_index[l]);
			}
		}

		// Last word

		// Transpose
		transpose64(transpose_index);

		// If it's last word we may have to mask the last bits that are noise
		word_t n_bits_to_check_mask = 0xffffffffffffffff
			<< (out_n_words * 64 - dm->n_matrix_lines);

		// Append to output buffer
		for (uint8_t l = 0; l < n_lines_to_write; l++)
		{
			transpose_index[l] &= n_bits_to_check_mask;

			out_buffer[l * out_n_words + ow] = transpose_index[l];

			// Update attribute totals
			attr_buffer[current_attribute_word * WORD_BITS + l]
				+= __builtin_popcountl(transpose_index[l]);
		}

		// Save transposed array to file
		hdf5_write_n_lines(dset_id, current_attribute_word * 64,
						   n_lines_to_write, out_n_words, H5T_NATIVE_UINT64,
						   out_buffer);
	}

	free(out_buffer);
	free(in_buffer);

	H5Dclose(dset_id);

	write_attribute_totals(totals_dset_id, dset->n_attributes, attr_buffer);

	free(attr_buffer);
	H5Dclose(totals_dset_id);

	return OK;
}

oknok_t write_attribute_totals(const hid_t dataset_id,
							   const uint32_t n_attributes,
							   const uint32_t* data)
{

	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2]  = { 1, n_attributes };

	hdf5_write_to_dataset(dataset_id, offset, count, H5T_NATIVE_UINT32, data);

	return OK;
}
