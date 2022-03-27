/*
 ============================================================================
 Name        : disjoint_matrix.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage the disjoint matrix
 ============================================================================
 */

#include "disjoint_matrix.h"

/**
 * Calculates the number of lines for the disjoint matrix
 */
uint_fast64_t calculate_number_of_lines(const uint_fast32_t *n_items_per_class) {
	// Calculate number of lines for the matrix
	uint_fast64_t n_lines = 0;

	for (uint_fast8_t i = 0; i < g_n_classes - 1; i++) {
		for (uint_fast8_t j = i + 1; j < g_n_classes; j++) {
			n_lines += n_items_per_class[i] * n_items_per_class[j];
		}
	}

	return n_lines;
}

/**
 * Builds the disjoint matrix and saves it to the hdf5 dataset file
 * If the dataset already exists it assumes it's filled from a previous run
 * and will do nothing
 */
herr_t create_disjoint_matrix(const char *filename, const char *datasetname,
		const uint_fast32_t *n_items_per_class,
		uint_fast64_t **observations_per_class) {

	// Open file
	hid_t file_id = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", filename);

		return NOK;
	}

	// Open dataset
	// If it exists assume it's filled
	// WHATIF: should we NOT assume it's filled and recalculate?

	hid_t dm_dataset_id = H5Dopen2(file_id, datasetname, H5P_DEFAULT);
	if (dm_dataset_id < 0) {
		// Dataset does not exist
		fprintf(stdout, "Matrix dataset not found. Creating new\n");

		if (create_new_disjoint_matrix_dataset(file_id, n_items_per_class,
				observations_per_class) != OK) {

			fprintf(stderr, "Error creating new disjoint matrix dataset\n");

			H5Fclose(file_id);

			return NOK;
		}
	}

	H5Fclose(file_id);

	return OK;
}

/**
 * Creates a new disjoint matrix dataset
 */
uint_fast8_t create_new_disjoint_matrix_dataset(hid_t file_id,
		const uint_fast32_t *n_items_per_class,
		uint_fast64_t **observations_per_class) {

	uint_fast64_t n_lines = calculate_number_of_lines(n_items_per_class);

	// Dataset dimensions
	hsize_t dm_dimensions[2] = { n_lines, g_n_longs };

	hid_t dm_dataset_space_id = H5Screate_simple(2, dm_dimensions, NULL);
	if (dm_dataset_space_id < 0) {
		// Error creating file
		fprintf(stderr, "Error creating dataset space\n");
		return NOK;
	}

	// Create a dataset creation property list
	hid_t dm_property_list_id = H5Pcreate(H5P_DATASET_CREATE);
	H5Pset_layout(dm_property_list_id, H5D_CHUNKED);

	// The choice of the chunk size affects performance!
	// for now we will choose one line
	hsize_t dm_chunk_dimensions[2] = { 1, g_n_longs };

	H5Pset_chunk(dm_property_list_id, 2, dm_chunk_dimensions);

	// Create the dataset
	hid_t dm_dataset_id = H5Dcreate2(file_id, DISJOINT_MATRIX_DATASET_NAME,
	H5T_STD_U64BE, dm_dataset_space_id, H5P_DEFAULT, dm_property_list_id,
	H5P_DEFAULT);
	if (dm_dataset_id < 0) {
		fprintf(stderr, "Error creating disjoint matrix dataset");
		return NOK;
	}

	// Close resources
	H5Pclose(dm_property_list_id);

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t dm_memory_space_id = H5Screate_simple(2, dm_chunk_dimensions,
	NULL);

	// Alocate buffer
	uint_fast64_t *buffer = (uint_fast64_t*) malloc(
			sizeof(uint_fast64_t) * dm_chunk_dimensions[1]);

	// We will write one line at a time
	hsize_t count[2] = { 1, dm_chunk_dimensions[1] };
	hsize_t offset[2] = { 0, 0 };

	// Current line
	for (uint_fast8_t i = 0; i < g_n_classes - 1; i++) {
		for (uint_fast8_t j = i + 1; j < g_n_classes; j++) {
			for (uint_fast32_t n_i = i * g_n_observations;
					n_i < i * g_n_observations + n_items_per_class[i]; n_i++) {
				for (uint_fast32_t n_j = j * g_n_observations;
						n_j < j * g_n_observations + n_items_per_class[j];
						n_j++) {
					for (uint_fast32_t n = 0; n < g_n_longs; n++) {
						buffer[n] = observations_per_class[n_i][n]
								^ observations_per_class[n_j][n];
					}

					// Select hyperslab on file dataset
					H5Sselect_hyperslab(dm_dataset_space_id, H5S_SELECT_SET,
							offset, NULL, count, NULL);

					// Write buffer to dataset
					H5Dwrite(dm_dataset_id, H5T_NATIVE_ULONG,
							dm_memory_space_id, dm_dataset_space_id,
							H5P_DEFAULT, buffer);

					// Update offset
					offset[0]++;
				}
			}
		}
	}

	free(buffer);

	H5Sclose(dm_memory_space_id);
	H5Sclose(dm_dataset_space_id);

	H5Dclose(dm_dataset_id);

	return OK;
}
