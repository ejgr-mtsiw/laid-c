/*
 ============================================================================
 Name        : set_cover.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to apply the set cover algorithm
 ============================================================================
 */

#include "set_cover.h"

/**
 * Applies the set cover algorithm to the hdf5 dataset and prints
 * the minimum attribute set that covers all the lines
 */
status_t calculate_solution(const char *filename, const char *datasetname,
		const uint_fast32_t *n_items_per_class) {

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

		// Free resources
		close_resources(file_id, 0, 0, 0);

		return NOK;
	}

	// Calculate number of lines of disjoint matrix
	// WHATIF: we could store this in the dataset as an attribute
	uint_fast32_t n_lines = calculate_number_of_lines(n_items_per_class);

	// Setup memory space
	hsize_t dm_dimensions[2] = { n_lines, g_n_longs };
	hid_t dm_dataset_space_id = H5Screate_simple(2, dm_dimensions, NULL);
	if (dm_dataset_space_id < 0) {
		// Error creating file
		fprintf(stderr, "Error creating dataset space\n");

		// Free resources
		close_resources(file_id, dataset_id, 0, 0);

		return NOK;
	}

	// The choice of the chunk size affects performance!
	// for now we will choose one line
	hsize_t dm_chunk_dimensions[2] = { 1, g_n_longs };

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t dm_memory_space_id = H5Screate_simple(2, dm_chunk_dimensions, NULL);

	// Update column totals
	/**
	 * Array to store the total number of '1's for each atribute
	 */
	uint_fast32_t *sum = calloc(g_n_attributes, sizeof(uint_fast32_t));
	if (sum == NULL) {
		fprintf(stderr, "Error allocating sum array\n");

		// Free resources
		close_resources(file_id, dataset_id, dm_dataset_space_id,
				dm_memory_space_id);

		return NOK;
	}

	/**
	 * Array to store the blacklisted attributes
	 * Also works as solution. Every attribute that's part of the
	 * solution is ignored in the next round of calculating totals
	 */
	uint_fast8_t *attribute_blacklist = calloc(g_n_attributes,
			sizeof(uint_fast8_t));
	if (attribute_blacklist == NULL) {
		fprintf(stderr, "Error allocating attribute blacklist array\n");

		// Free resources
		close_resources(file_id, dataset_id, dm_dataset_space_id,
				dm_memory_space_id);

		return NOK;
	}

	/**
	 * Allocate blacklisted lines array
	 */
	uint_fast8_t *line_blacklist = calloc(n_lines, sizeof(uint_fast8_t));
	if (line_blacklist == NULL) {
		fprintf(stderr, "Error allocating line blacklist array\n");

		// Free resources
		close_resources(file_id, dataset_id, dm_dataset_space_id,
				dm_memory_space_id);

		return NOK;
	}

	uint_fast32_t max_total = 0;

	do {

		// Reset totals
		memset(sum, 0, sizeof(uint_fast32_t) * g_n_attributes);

		// Update sum array
		update_sum(dataset_id, dm_dataset_space_id, dm_memory_space_id, n_lines,
				line_blacklist, attribute_blacklist, sum);

		// Select attribute to blacklist / add to solution
		uint_fast32_t attribute_to_blacklist = 0;

		// Calculate max total
		max_total = sum[0];
		for (uint_fast32_t i = 1; i < g_n_attributes; i++) {
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

		// Blacklist attribute with max total
		attribute_blacklist[attribute_to_blacklist] = 1;
		fprintf(stdout, "  - Blacklisted: %lu\n", attribute_to_blacklist + 1);

		// Blacklist lines that have the blacklisted attribute set
		blacklist_lines(dataset_id, dm_dataset_space_id, dm_memory_space_id,
				n_lines, attribute_to_blacklist, line_blacklist);

	} while (max_total > 0);

	free(sum);
	free(line_blacklist);

	// Free resources
	close_resources(file_id, dataset_id, dm_dataset_space_id,
			dm_memory_space_id);

	fprintf(stdout, "Solution: { ");
	for (uint_fast32_t i = 0; i < g_n_attributes; i++) {
		if (attribute_blacklist[i]) {
			// This attribute is set so it's part of the solution
			printf("%lu ", i + 1);
		}
	}
	printf("}\n");

	free(attribute_blacklist);

	return OK;
}

status_t blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const uint_fast64_t n_lines,
		const uint_fast32_t attribute_to_blacklist,
		uint_fast8_t *line_blacklist) {

	// Attribute to blacklist is in long n
	uint_fast32_t n = attribute_to_blacklist / BLOCK_BITS;

	// Attribute to blacklist is at
	uint_fast8_t bit = BLOCK_BITS - 1 - attribute_to_blacklist % BLOCK_BITS;

	// Alocate buffer
	uint_fast64_t *buffer = (uint_fast64_t*) malloc(
			sizeof(uint_fast64_t) * g_n_longs);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, g_n_longs };

	uint_fast32_t n_blacklisted_lines = 0;

	for (uint_fast32_t i = 0; i < n_lines; i++) {

		if (line_blacklist[i] == BLACKLISTED) {
			continue;
		}

		// Update offset
		offset[0] = i;

		// Select hyperslab on file dataset
		H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset,
		NULL, count, NULL);

		// Read line to dataset
		H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id, dataset_space_id,
		H5P_DEFAULT, buffer);

		if (buffer[n] & AND_MASK_TABLE[bit]) {
			// The bit is set: Blacklist this line
			line_blacklist[i] = BLACKLISTED;
			n_blacklisted_lines++;
		}
	}

#ifdef DEBUG
	fprintf(stdout, "Blacklisted %lu lines.\n", n_blacklisted_lines);
#endif

	return OK;
}

/**
 * Finds the next attribute to blacklist
 */
uint_fast32_t update_sum(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const uint_fast32_t n_lines,
		const uint_fast8_t *line_blacklist,
		const uint_fast8_t *attribute_blacklist, uint_fast32_t *sum) {

	// Alocate buffer
	uint_fast64_t *buffer = (uint_fast64_t*) malloc(
			sizeof(uint_fast64_t) * g_n_longs);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, g_n_longs };

	// Calculate totals
	for (uint_fast32_t i = 0; i < n_lines; i++) {

		// If this line is blacklisted: skip!
		if (line_blacklist[i] == BLACKLISTED) {
			continue;
		}

		// Update offset
		offset[0] = i;

		// Select hyperslab on file dataset
		H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset,
		NULL, count, NULL);

		// Read line to dataset
		H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id, dataset_space_id,
		H5P_DEFAULT, buffer);

		// Current attribute
		uint_fast32_t c = 0;

		for (uint_fast32_t n = 0; n < g_n_longs && c < g_n_attributes; n++) {

			for (int_fast8_t bit = BLOCK_BITS - 1;
					c < g_n_attributes && bit >= 0; bit--) {

				if (attribute_blacklist[c] == NOT_BLACKLISTED
						&& (buffer[n] & AND_MASK_TABLE[bit])) {
					// Add to cost
					sum[c]++;
				}
				c++;
			}
		}

#ifdef DEBUG
		if ((i + 1) % (n_lines / 10 + 1) == 0) {
			fprintf(stdout, "DMX: Analysing line %lu of %lu.\n", i + 1,
					n_lines);
		}
#endif

	}

	free(buffer);

	return OK;
}

void close_resources(hid_t file_id, hid_t dataset_id, hid_t dm_dataset_space_id,
		hid_t dm_memory_space_id) {

	// Free resources
	if (dm_memory_space_id != 0) {
		H5Sclose(dm_memory_space_id);
	}

	if (dm_dataset_space_id != 0) {
		H5Sclose(dm_dataset_space_id);
	}

	if (dataset_id != 0) {
		H5Dclose(dataset_id);
	}

	if (file_id != 0) {
		H5Fclose(file_id);
	}
}
