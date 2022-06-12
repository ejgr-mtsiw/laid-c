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
herr_t calculate_solution(const char *filename, const char *datasetname,
		cover_t *cover) {

	// Open file
	hid_t file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", filename);

		return -1;
	}

	// Open dataset
	hid_t dataset_id = H5Dopen2(file_id, datasetname, H5P_DEFAULT);
	if (dataset_id < 1) {
		// Error creating file
		fprintf(stderr, "Dataset %s not found!\n", datasetname);

		// Free resources
		close_resources(file_id, 0, 0, 0);

		return -1;
	}

	// Number of longs in a line
	unsigned int n_longs = cover->dataset->n_longs;

	// Number of attributes
	unsigned int n_attributes = cover->dataset->n_attributes;

	// Number of observations
//	unsigned int n_observations = cover->dataset->n_observations;

// Number of classes
//	unsigned int n_classes = cover->dataset->n_classes;

// Observations per class
//	unsigned long **observations_per_class =
//			cover->dataset->observations_per_class;

// Number of observations per class
//	unsigned int *n_observations_per_class =
//			cover->dataset->n_observations_per_class;

// Calculate number of lines of disjoint matrix
	unsigned long n_lines = cover->matrix_n_lines;

	// Setup memory space
	hsize_t dm_dimensions[2] = { n_lines, n_longs };
	hid_t dm_dataset_space_id = H5Screate_simple(2, dm_dimensions, NULL);
	if (dm_dataset_space_id < 0) {
		// Error creating file
		fprintf(stderr, "Error creating dataset space\n");

		// Free resources
		close_resources(file_id, dataset_id, 0, 0);

		return -1;
	}

	// The choice of the chunk size affects performance!
	// for now we will choose one line
	hsize_t dm_chunk_dimensions[2] = { 1, n_longs };

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t dm_memory_space_id = H5Screate_simple(2, dm_chunk_dimensions, NULL);

	// Update column totals
	/**
	 * Array to store the blacklisted attributes
	 * Also works as solution. Every attribute that's part of the
	 * solution is ignored in the next round of calculating totals
	 */
	cover->attribute_blacklist = calloc(n_attributes, sizeof(unsigned char));
	if (cover->attribute_blacklist == NULL) {
		fprintf(stderr, "Error allocating attribute blacklist array\n");

		// Free resources
		close_resources(file_id, dataset_id, dm_dataset_space_id,
				dm_memory_space_id);

		return -1;
	}

	/**
	 * Allocate blacklisted lines array
	 */
	cover->line_blacklist = calloc(n_lines, sizeof(unsigned char));
	if (cover->line_blacklist == NULL) {
		fprintf(stderr, "Error allocating line blacklist array\n");

		// Free resources
		close_resources(file_id, dataset_id, dm_dataset_space_id,
				dm_memory_space_id);

		return -1;
	}

	/**
	 * Array to store the total number of '1's for each atribute
	 */
	cover->sum = calloc(n_attributes, sizeof(unsigned int));
	if (cover->sum == NULL) {
		fprintf(stderr, "Error allocating sum array\n");

		// Free resources
		close_resources(file_id, dataset_id, dm_dataset_space_id,
				dm_memory_space_id);

		return -1;
	}

	unsigned int *sum = cover->sum;

	// Calculate initial sum for each attribute
	calculate_initial_sum(dataset_id, dm_dataset_space_id, dm_memory_space_id,
			cover);

	unsigned int max_total = 0;
	unsigned int attribute_to_blacklist = 0;

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

		if (max_total <= 0) {
			// Nothing else to do here: we have a solution that covers the
			// full disjoint matrix
			break;
		}

		// Blacklist attribute with max total
		cover->attribute_blacklist[attribute_to_blacklist] = 1;
		fprintf(stdout, "  - Blacklisted: %d\n", attribute_to_blacklist + 1);
		fflush( stdout);

		// Blacklist lines that have the blacklisted attribute set
		blacklist_lines(dataset_id, dm_dataset_space_id, dm_memory_space_id,
				cover, attribute_to_blacklist);

	} while (max_total > 0);

	free(cover->sum);
	free(cover->line_blacklist);

	// Free resources
	close_resources(file_id, dataset_id, dm_dataset_space_id,
			dm_memory_space_id);

	fprintf(stdout, "Solution: { ");
	for (unsigned int i = 0; i < n_attributes; i++) {
		if (cover->attribute_blacklist[i]) {
			// This attribute is set so it's part of the solution
			printf("%d ", i + 1);
		}
	}
	printf("}\n");

	free(cover->attribute_blacklist);

	return 0;
}

herr_t blacklist_lines(const hid_t dataset_id, const hid_t dataset_space_id,
		const hid_t memory_space_id, const cover_t *cover,
		const unsigned int attribute_to_blacklist) {

#ifdef DEBUG
	unsigned int next_output = 0;
	fprintf(stdout, "[set_cover::blacklist_lines] Starting.\n");
	fflush( stdout);
#endif

	// Attribute to blacklist is in long n
	unsigned int n = attribute_to_blacklist / BLOCK_BITS;

	// Attribute to blacklist is at
	unsigned char bit = BLOCK_BITS - 1 - attribute_to_blacklist % BLOCK_BITS;

	// Number of longs in a line
	unsigned int n_longs = cover->dataset->n_longs;

	// Number of attributes
	unsigned int n_attributes = cover->dataset->n_attributes;

	// Number of observations
//	unsigned int n_observations = cover->dataset->n_observations;

	// Number of classes
//	unsigned int n_classes = cover->dataset->n_classes;

	// Calculate number of lines of disjoint matrix
	unsigned long n_lines = cover->matrix_n_lines;

	unsigned char *line_blacklist = cover->line_blacklist;

//	unsigned char *attribute_blacklist = cover->attribute_blacklist;

	unsigned int *sum = cover->sum;

	// Alocate buffer
	unsigned long *buffer = (unsigned long*) malloc(
			sizeof(unsigned long) * n_longs);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, n_longs };

	unsigned int n_blacklisted_lines = 0;

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
				n_blacklisted_lines++;

				// Subtract this line contrinution from total
				unsigned int c = 0;

				for (unsigned int l = 0; l < n_longs; l++) {

					if (buffer[l] == 0UL) {
						c += BLOCK_BITS;
						continue;
					}

					for (int bit = BLOCK_BITS - 1; c < n_attributes && bit >= 0;
							bit--, c++) {

						if (buffer[l] & AND_MASK_TABLE[bit]) {
							// Add to sum
							sum[c]--;
						}
					}
				}
			}
		}

#ifdef DEBUG
		if (i > next_output) {
			fprintf(stdout, "[set_cover::blacklist_lines] %0.0f %%.\n",
					((double) i) / n_lines * 100);
			fflush( stdout);

			next_output += n_lines / 10;
		}
#endif

	}

#ifdef DEBUG
	fprintf(stdout, "Blacklisted %d lines.\n", n_blacklisted_lines);
	fflush( stdout);
#endif

	return 0;
}

/**
 * Finds the next attribute to blacklist
 */
unsigned int calculate_initial_sum(const hid_t dataset_id,
		const hid_t dataset_space_id, const hid_t memory_space_id,
		const cover_t *cover) {

#ifdef DEBUG
	unsigned int next_output = 0;
	fprintf(stdout, "[set_cover::update_sum] Starting.\n");
	fflush( stdout);
#endif

	unsigned int n_longs = cover->dataset->n_longs;

	unsigned long n_lines = cover->matrix_n_lines;

	unsigned int n_attributes = cover->dataset->n_attributes;

	unsigned int *sum = cover->sum;

	// Alocate buffer
	unsigned long *buffer = (unsigned long*) malloc(
			sizeof(unsigned long) * n_longs);

	// We will read one line at a time
	hsize_t offset[2] = { 0, 0 };
	hsize_t count[2] = { 1, n_longs };

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

		// Current attribute
		unsigned int c = 0;

		for (unsigned int n = 0; n < n_longs; n++) {
			for (int bit = BLOCK_BITS - 1; c < n_attributes && bit >= 0;
					bit--, c++) {

				if (buffer[n] & AND_MASK_TABLE[bit]) {
					// Add to sum
					sum[c]++;
				}
			}
		}

#ifdef DEBUG
		if (i > next_output) {
			fprintf(stdout, "[set_cover::update_sum] %0.0f%%.\n",
					((double) i) / n_lines * 100);
			fflush( stdout);

			next_output += n_lines / 10;
		}
#endif

	}

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
