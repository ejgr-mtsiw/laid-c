/*
 ============================================================================
 Name        : set_cover.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to apply the set cover algorithm
 ============================================================================
 */

#include "set_cover.h"

/**
 * Applies the set cover algorithm to the hdf5 dataset and prints
 * the minimum attribute set that covers all the lines
 */
oknok_t calculate_solution(const char *filename, const char *datasetname,
		cover_t *cover) {

	oknok_t ret = OK;

	hid_t file_id, dataset_id;

	ret = hdf5_open_dataset(filename, datasetname, &file_id, &dataset_id);
	if (ret != OK) {
		return ret;
	}

	hsize_t dm_dimensions[2];
	hdf5_get_dataset_dimensions(dataset_id, dm_dimensions);

	// Setup memory space
	hid_t dm_dataset_space_id = H5Screate_simple(2, dm_dimensions, NULL);
	if (dm_dataset_space_id < 0) {
		// Error creating file
		fprintf(stderr, "Error creating dataset space\n");

		ret = NOK;
		goto out_close_dataset;
	}

	// Get number of lines of disjoint matrix
	uint32_t n_lines = dm_dimensions[0];

	// Number of longs in a line
	uint32_t n_words = dm_dimensions[1];

	// Number of attributes
	uint32_t n_attributes = 0;
	hdf5_read_attribute(dataset_id, HDF5_N_ATTRIBUTES_ATTRIBUTE,
	H5T_NATIVE_UINT, &n_attributes);

	// The choice of the chunk size affects performance!
	// for now we will choose one line
	hsize_t dm_chunk_dimensions[2] = { 1, n_words };

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t dm_memory_space_id = H5Screate_simple(2, dm_chunk_dimensions, NULL);

	cover->n_words = n_words;
	cover->matrix_n_lines = n_lines;
	cover->n_attributes = n_attributes;

	// Update column totals
	/**
	 * Array to store the blacklisted attributes
	 * Also works as solution. Every attribute that's part of the
	 * solution is ignored in the next round of calculating totals
	 */
	cover->attribute_blacklist = calloc(n_attributes, sizeof(uint8_t));
	if (cover->attribute_blacklist == NULL) {
		fprintf(stderr, "Error allocating attribute blacklist array\n");

		ret = NOK;
		goto out_close_memory_space;
	}

	/**
	 * Allocate blacklisted lines array
	 */
	cover->line_blacklist = calloc(n_lines, sizeof(uint8_t));
	if (cover->line_blacklist == NULL) {
		fprintf(stderr, "Error allocating line blacklist array\n");

		ret = NOK;
		goto out_close_memory_space;
	}

	/**
	 * Array to store the total number of '1's for each atribute
	 */
	cover->sum = calloc(n_attributes, sizeof(uint32_t));
	if (cover->sum == NULL) {
		fprintf(stderr, "Error allocating sum array\n");

		ret = NOK;
		goto out_close_memory_space;
	}

	// Calculate initial sum for each attribute
	calculate_initial_sum(dataset_id, dm_dataset_space_id, dm_memory_space_id,
			cover);

	uint32_t max_total = 0;
	uint32_t attribute_to_blacklist = 0;
	uint32_t *sum = cover->sum;

	uint32_t round = 1;

	do {

		// Select attribute to blacklist / add to solution
		max_total = sum[0];
		attribute_to_blacklist = 0;

		for (uint32_t i = 1; i < n_attributes; i++) {
			if (sum[i] > max_total) {
				max_total = sum[i];
				attribute_to_blacklist = i;
			}
		}

		if (max_total == 0) {
			// Nothing else to do here: we have a solution that covers the
			// full disjoint matrix
			break;
		}

		fprintf(stdout, "\n - Selecting attribute round #%d\n", round++);

		// Blacklist attribute with max total
		cover->attribute_blacklist[attribute_to_blacklist] = BLACKLISTED;
		fprintf(stdout, "  - Selected attribute #%d\n", attribute_to_blacklist);

		// Blacklist lines that have the blacklisted attribute set
		blacklist_lines(dataset_id, dm_dataset_space_id, dm_memory_space_id,
				cover, attribute_to_blacklist);

	} while (max_total > 0);

out_close_memory_space:
	H5Sclose(dm_memory_space_id);
	H5Sclose(dm_dataset_space_id);

out_close_dataset:
	H5Dclose(dataset_id);
	H5Fclose(file_id);

	free(cover->sum);
	free(cover->line_blacklist);

	cover->sum = NULL;
	cover->line_blacklist = NULL;

	return ret;
}

herr_t blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const cover_t *cover,
		const uint32_t attribute_to_blacklist) {

	SETUP_TIMING
	TICK

	// Attribute to blacklist is in long n
	uint32_t n = attribute_to_blacklist / WORD_BITS;

	// Attribute to blacklist is at
	uint8_t bit = WORD_BITS - 1 - attribute_to_blacklist % WORD_BITS;

	// Number of longs in a line
	uint32_t n_words = cover->n_words;

	// Number of attributes
	uint32_t n_attributes = cover->n_attributes;

	// Calculate number of lines of disjoint matrix
	uint32_t n_lines = cover->matrix_n_lines;

	uint8_t *line_blacklist = cover->line_blacklist;

	uint32_t *sum = cover->sum;

	// Alocate buffer
	word_t *buffer = (word_t*) malloc(sizeof(word_t) * n_words);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, n_words };

	// Current long
	word_t c_long = 0;

	// Current attribute
	uint32_t c_attribute = 0;

	const uint32_t n_lines_to_blacklist = sum[attribute_to_blacklist];
	uint32_t next_output = 0;

	for (uint32_t i = 0; i < n_lines; i++) {

		if (line_blacklist[i] != BLACKLISTED) {

			// Update offset
			offset[0] = i;

			// Select hyperslab on file dataset
			H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset,
			NULL, count, NULL);

			// Read line to dataset
			H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id,
					dataset_space_id,
					H5P_DEFAULT, buffer);

			if (buffer[n] & AND_MASK_TABLE[bit]) {

				// The bit is set: Blacklist this line
				line_blacklist[i] = BLACKLISTED;

				// Update sum removing the contribution from this line
				for (uint32_t l = 0; l < n_words; l++) {

					c_long = buffer[l];
					c_attribute = l * WORD_BITS;

					for (int bit = WORD_BITS - 1;
							c_attribute < n_attributes && bit >= 0;
							bit--, c_attribute++) {

						// Update sum
						sum[c_attribute] -= !!(c_long & AND_MASK_TABLE[bit]);
					}
				}
			}
		}

		if (i > next_output) {
			fprintf(stdout, "  - Blacklisting lines %0.0f%%.\r",
					((double) i) / n_lines * 100);
			fflush( stdout);

			next_output += n_lines / 10;
		}
	}

	fprintf(stdout, "  - Blacklisted %d lines ", n_lines_to_blacklist);
	TOCK(stdout)

	free(buffer);

	return 0;
}

/**
 * Finds the next attribute to blacklist
 */
void calculate_initial_sum(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const cover_t *cover) {

	SETUP_TIMING
	TICK

	uint32_t n_words = cover->n_words;

	unsigned long n_lines = cover->matrix_n_lines;

	uint32_t n_attributes = cover->n_attributes;

	uint32_t *sum = cover->sum;

	// Alocate buffer
	word_t *buffer = (word_t*) malloc(sizeof(word_t) * n_words);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, n_words };

	// Current long
	word_t c_long = 0;

	// Current attribute
	uint32_t c_attribute = 0;

	uint32_t next_output = 0;

	// Calculate totals
	for (uint32_t i = 0; i < n_lines; i++) {

		// Update offset
		offset[0] = i;

		// Select hyperslab on file dataset
		H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset,
		NULL, count, NULL);

		// Read line to dataset
		H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id, dataset_space_id,
		H5P_DEFAULT, buffer);

		for (uint32_t l = 0; l < n_words; l++) {
			c_long = buffer[l];
			c_attribute = l * WORD_BITS;

			for (int bit = WORD_BITS - 1;
					bit >= 0 && c_attribute < n_attributes;
					bit--, c_attribute++) {

				// Add to sum
				sum[c_attribute] += !!(c_long & AND_MASK_TABLE[bit]);
			}
		}

		if (i > next_output) {
			fprintf(stdout, " - Computing initial sum %0.0f%%.\r",
					((double) i) / n_lines * 100);
			fflush( stdout);

			next_output += n_lines / 10;
		}

	}

	fprintf(stdout, " - Calculated initial sum ");
	TOCK(stdout)

	free(buffer);
}

void print_solution(FILE *stream, cover_t *cover) {

	fprintf(stream, "\nSolution: { ");
	for (uint32_t i = 0; i < cover->n_attributes; i++) {
		if (cover->attribute_blacklist[i] == BLACKLISTED) {
			// This attribute is set so it's part of the solution
			fprintf(stream, "%d ", i);
		}
	}
	fprintf(stream, "}\n");
}

void free_cover(cover_t *cover) {

	free(cover->attribute_blacklist);
	free(cover->line_blacklist);
	free(cover->sum);

	cover->attribute_blacklist = NULL;
	cover->line_blacklist = NULL;
	cover->sum = NULL;
}
