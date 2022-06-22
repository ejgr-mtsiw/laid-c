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
int calculate_solution(const char *filename, const char *datasetname,
		cover_t *cover) {

	int ret = OK;

	// Open file
	hid_t file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", filename);

		return NOK;
	}

	// Open dataset
	hid_t dataset_id = H5Dopen2(file_id, datasetname, H5P_DEFAULT);
	if (dataset_id < 1) {
		// Error creating file
		fprintf(stderr, "Dataset %s not found!\n", datasetname);

		ret = NOK;
		goto out_close_file;
	}

	hsize_t dm_dimensions[2];
	hdf5_get_dataset_dimensions(dataset_id, dm_dimensions);

	// Get number of lines of disjoint matrix
	unsigned long n_lines = dm_dimensions[0];

	// Number of longs in a line
	unsigned int n_longs = dm_dimensions[1];

	// Number of attributes
	unsigned int n_attributes = 0;
	hdf5_read_attribute(dataset_id, HDF5_N_ATTRIBUTES_ATTRIBUTE,
	H5T_NATIVE_UINT, &n_attributes);

	// Setup memory space
	//hsize_t dm_dimensions[2] = { n_lines, n_longs };
	hid_t dm_dataset_space_id = H5Screate_simple(2, dm_dimensions, NULL);
	if (dm_dataset_space_id < 0) {
		// Error creating file
		fprintf(stderr, "Error creating dataset space\n");

		ret = NOK;
		goto out_close_dataset;
	}

	// The choice of the chunk size affects performance!
	// for now we will choose one line
	hsize_t dm_chunk_dimensions[2] = { 1, n_longs };

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t dm_memory_space_id = H5Screate_simple(2, dm_chunk_dimensions, NULL);

	cover->n_longs = n_longs;
	cover->matrix_n_lines = n_lines;
	cover->n_attributes = n_attributes;

	// Update column totals
	/**
	 * Array to store the blacklisted attributes
	 * Also works as solution. Every attribute that's part of the
	 * solution is ignored in the next round of calculating totals
	 */
	cover->attribute_blacklist = calloc(n_attributes, sizeof(unsigned char));
	if (cover->attribute_blacklist == NULL) {
		fprintf(stderr, "Error allocating attribute blacklist array\n");

		ret = NOK;
		goto out_close_memory_space;
	}

	/**
	 * Allocate blacklisted lines array
	 */
	cover->line_blacklist = calloc(n_lines, sizeof(unsigned char));
	if (cover->line_blacklist == NULL) {
		fprintf(stderr, "Error allocating line blacklist array\n");

		ret = NOK;
		goto out_close_memory_space;
	}

	/**
	 * Array to store the total number of '1's for each atribute
	 */
	cover->sum = calloc(n_attributes, sizeof(unsigned int));
	if (cover->sum == NULL) {
		fprintf(stderr, "Error allocating sum array\n");

		ret = NOK;
		goto out_close_memory_space;
	}

	// Calculate initial sum for each attribute
	calculate_initial_sum(dataset_id, dm_dataset_space_id, dm_memory_space_id,
			cover);

	unsigned int max_total = 0;
	unsigned int attribute_to_blacklist = 0;
	unsigned int *sum = cover->sum;

	unsigned int round = 1;

	do {

		// Select attribute to blacklist / add to solution
		max_total = sum[0];
		attribute_to_blacklist = 0;

		for (unsigned int i = 1; i < n_attributes; i++) {
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

	out_close_memory_space: H5Sclose(dm_memory_space_id);

	H5Sclose(dm_dataset_space_id);

	out_close_dataset: H5Dclose(dataset_id);

	out_close_file: H5Fclose(file_id);

	free(cover->sum);
	free(cover->line_blacklist);

	cover->sum = NULL;
	cover->line_blacklist = NULL;

	return ret;
}

herr_t blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const cover_t *cover,
		const unsigned int attribute_to_blacklist) {

	SETUP_TIMING
	TICK

	// Attribute to blacklist is in long n
	unsigned int n = attribute_to_blacklist / BLOCK_BITS;

	// Attribute to blacklist is at
	unsigned char bit = BLOCK_BITS - 1 - attribute_to_blacklist % BLOCK_BITS;

	// Number of longs in a line
	unsigned int n_longs = cover->n_longs;

	// Number of attributes
	unsigned int n_attributes = cover->n_attributes;

	// Calculate number of lines of disjoint matrix
	unsigned long n_lines = cover->matrix_n_lines;

	unsigned char *line_blacklist = cover->line_blacklist;

	unsigned int *sum = cover->sum;

	// Alocate buffer
	unsigned long *buffer = (unsigned long*) malloc(
			sizeof(unsigned long) * n_longs);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, n_longs };

	// Current long
	unsigned long c_long = 0;

	// Current attribute
	unsigned long c_attribute = 0;

#ifdef DEBUG
	const unsigned int n_lines_to_blacklist = sum[attribute_to_blacklist];
	unsigned int next_output = 0;
#endif

	for (unsigned int i = 0; i < n_lines; i++) {

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
				for (unsigned int l = 0; l < n_longs; l++) {

					c_long = buffer[l];
					c_attribute = l * BLOCK_BITS;

					for (int bit = BLOCK_BITS - 1;
							c_attribute < n_attributes && bit >= 0;
							bit--, c_attribute++) {

						// Update sum
						sum[c_attribute] -= !!(c_long & AND_MASK_TABLE[bit]);
					}
				}
			}
		}

#ifdef DEBUG
		if (i > next_output) {
			fprintf(stdout, "  - Blacklisting lines %0.0f%%.\r",
					((double) i) / n_lines * 100);
			fflush( stdout);

			next_output += n_lines / 10;
		}
#endif
	}

#ifdef DEBUG
	fprintf(stdout, "  - Blacklisted %d lines ", n_lines_to_blacklist);
	TOCK(stdout)
#endif

	free(buffer);

	return 0;
}

/**
 * Finds the next attribute to blacklist
 */
unsigned int calculate_initial_sum(const hid_t dataset_id,
		const hid_t dataset_space_id, const hid_t memory_space_id,
		const cover_t *cover) {

	SETUP_TIMING
	TICK

	unsigned int n_longs = cover->n_longs;

	unsigned long n_lines = cover->matrix_n_lines;

	unsigned int n_attributes = cover->n_attributes;

	unsigned int *sum = cover->sum;

	// Alocate buffer
	unsigned long *buffer = (unsigned long*) malloc(
			sizeof(unsigned long) * n_longs);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, n_longs };

	// Current long
	unsigned long c_long = 0;

	// Current attribute
	unsigned int c_attribute = 0;

#ifdef DEBUG
	unsigned int next_output = 0;
#endif

	// Calculate totals
	for (unsigned int i = 0; i < n_lines; i++) {

		// Update offset
		offset[0] = i;

		// Select hyperslab on file dataset
		H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset,
		NULL, count, NULL);

		// Read line to dataset
		H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id, dataset_space_id,
		H5P_DEFAULT, buffer);

		for (unsigned int l = 0; l < n_longs; l++) {
			c_long = buffer[l];
			c_attribute = l * BLOCK_BITS;

			for (int bit = BLOCK_BITS - 1;
					bit >= 0 && c_attribute < n_attributes;
					bit--, c_attribute++) {

				// Add to sum
				sum[c_attribute] += !!(c_long & AND_MASK_TABLE[bit]);
			}
		}

#ifdef DEBUG
		if (i > next_output) {
			fprintf(stdout, " - Computing initial sum %0.0f%%.\r",
					((double) i) / n_lines * 100);
			fflush( stdout);

			next_output += n_lines / 10;
		}
#endif

	}

#ifdef DEBUG
	fprintf(stdout, " - Calculated initial sum ");
	TOCK(stdout)
#endif

	free(buffer);

	return 0;
}
//
//void update_sum(const unsigned long *buffer,
//		const unsigned char *attribute_blacklist, unsigned int *sum) {
//
//	// Current attribute
//	unsigned int c = 0;
//
//	for (unsigned int n = 0; n < g_n_longs; n++) {
//		for (char bit = BLOCK_BITS - 1; c < g_n_attributes && bit >= 0;
//				bit--, c++) {
//
//			if (attribute_blacklist[c] == NOT_BLACKLISTED
//					&& (buffer[n] & AND_MASK_TABLE[bit])) {
//				// Add to sum
//				sum[c]++;
//			}
//		}
//	}
//}

void print_solution(FILE *stream, cover_t *cover) {

	fprintf(stream, "\nSolution: { ");
	for (unsigned int i = 0; i < cover->n_attributes; i++) {
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
