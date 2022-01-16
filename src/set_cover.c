/*
 ============================================================================
 Name        : cost.h
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
		const unsigned long *n_items_per_class) {

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
		H5Fclose(file_id);

		return NOK;
	}

	// Calculate number of lines of disjoint matrix
	// WHATIF: we could store this in the dataset as an attribute
	unsigned long n_lines = calculate_number_of_lines(n_items_per_class);

	// Setup memory space
	hsize_t dm_dimensions[2] = { n_lines, g_n_longs };
	hid_t dm_dataset_space_id = H5Screate_simple(2, dm_dimensions, NULL);
	if (dm_dataset_space_id < 0) {
		// Error creating file
		fprintf(stderr, "Error creating dataset space\n");

		// Free resources
		H5Dclose(dataset_id);
		H5Fclose(file_id);

		return NOK;
	}

	// The choice of the chunk size affects performance!
	// for now we will choose one line
	hsize_t dm_chunk_dimensions[2] = { 1, g_n_longs };

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t dm_memory_space_id = H5Screate_simple(2, dm_chunk_dimensions, NULL);

	// Update cost
	/**
	 * Array to store the costs for each atribute
	 */
	unsigned long *cost = calloc(g_n_attributes, sizeof(long));
	if (cost == NULL) {
		fprintf(stderr, "Error allocating cost array\n");

		// Free resources
		H5Sclose(dm_memory_space_id);
		H5Sclose(dm_dataset_space_id);
		H5Dclose(dataset_id);
		H5Fclose(file_id);

		return NOK;
	}

	/**
	 * Array to store the blacklisted attributes
	 * Also works as solution. Every attribute that's part
	 * of the solution is ignored in the next round of calculating costs
	 */
	unsigned char *attribute_blacklist = calloc(g_n_attributes,
			sizeof(unsigned char));
	if (attribute_blacklist == NULL) {
		fprintf(stderr, "Error allocating attribute blacklist array\n");

		// Free resources
		H5Sclose(dm_memory_space_id);
		H5Sclose(dm_dataset_space_id);
		H5Dclose(dataset_id);
		H5Fclose(file_id);

		return NOK;
	}

	/**
	 * Allocate blacklisted lines array
	 */
	unsigned char *line_blacklist = calloc(n_lines, sizeof(unsigned char));
	if (line_blacklist == NULL) {
		fprintf(stderr, "Error allocating line blacklist array\n");

		// Free resources
		H5Sclose(dm_memory_space_id);
		H5Sclose(dm_dataset_space_id);
		H5Dclose(dataset_id);
		H5Fclose(file_id);

		return NOK;
	}

	unsigned long max_cost = 0;

	do {

		memset(cost, 0, sizeof(unsigned long) * g_n_attributes);

		// Update cost array
		calculate_cost(dataset_id, dm_dataset_space_id, dm_memory_space_id,
				n_lines, line_blacklist, attribute_blacklist, cost);

		// Select attribute to blacklist / add to solution
		unsigned long attribute_to_blacklist = 0;

		// Calculate max cost
		max_cost = cost[0];
		for (unsigned long i = 1; i < g_n_attributes; i++) {
			if (cost[i] > max_cost) {
				max_cost = cost[i];
				attribute_to_blacklist = i;
			}
		}

		if (max_cost == 0) {
			// Nothing else to do here: we have a solution that covers the
			// full disjoint matrix
			break;
		}

		// Blacklist attribute with max cost
		attribute_blacklist[attribute_to_blacklist] = 1;
		cost[attribute_to_blacklist] = 0;
		fprintf(stdout, "  - Blacklisted: %lu\n", attribute_to_blacklist + 1);

		// Blacklist lines that have this attribute
		blacklist_lines(dataset_id, dm_dataset_space_id, dm_memory_space_id,
				n_lines, attribute_to_blacklist, line_blacklist);

	} while (max_cost > 0);

	free(cost);
	free(line_blacklist);

	// Free resources
	H5Sclose(dm_memory_space_id);
	H5Sclose(dm_dataset_space_id);
	H5Dclose(dataset_id);
	H5Fclose(file_id);

	fprintf(stdout, "Solution: { ");
	for (unsigned long i = 0; i < g_n_attributes; i++) {
		if (attribute_blacklist[i]) {
			// This attribute is set so it's part of the solution
			printf("%lu ", i + 1);
		}
	}
	printf("}\n");

	free(attribute_blacklist);

	return OK;
}

int blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const unsigned long n_lines,
		const unsigned long attribute_to_blacklist,
		unsigned char *line_blacklist) {

// Attribute to blacklist is in long n
	unsigned int n = attribute_to_blacklist / LONG_BITS;

// Attribute to blacklist is at
	unsigned long bit = LONG_BITS - 1 - attribute_to_blacklist % LONG_BITS;

	// Alocate buffer
	unsigned long *buffer = (unsigned long*) malloc(
			sizeof(unsigned long) * g_n_longs);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, g_n_longs };

	for (unsigned long i = 0; i < n_lines; i++) {

		if (line_blacklist[i]) {
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
			// Disagreement
			// Blacklist this line
			line_blacklist[i] = 1;
		}

		if ((i % (n_lines / 10)) == 0) {
			fprintf(stdout, "... %07lu\n", i);
		}
	}

	return OK;
}
