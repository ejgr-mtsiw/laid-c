/*
 ============================================================================
 Name        : disjoint_matrix.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage the disjoint matrix
 ============================================================================
 */

#include "disjoint_matrix.h"

uint32_t calculate_number_of_lines_of_disjoint_matrix(const dataset_t *dataset) {

	// Calculate number of lines for the matrix
	uint32_t n = 0;

	uint32_t n_classes = dataset->n_classes;
	uint32_t *n_observations_per_class = dataset->n_observations_per_class;

	for (uint32_t i = 0; i < n_classes - 1; i++) {
		for (uint32_t j = i + 1; j < n_classes; j++) {
			n += n_observations_per_class[i] * n_observations_per_class[j];
		}
	}

	return n;
}

bool is_matrix_created(const char *filename) {

	return hdf5_dataset_exists_in_file(filename,
	DM_DATASET_ATTRIBUTES_COLUMN) && hdf5_dataset_exists_in_file(filename,
	DM_DATASET_ATTRIBUTES_LINE);
}

oknok_t create_disjoint_matrix(const char *filename, const dataset_t *dataset) {

	oknok_t ret = OK;

	// Open file
	hid_t file_id = H5Fopen(filename, H5F_ACC_RDWR, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", filename);

		return NOK;
	}

	if (create_disjoint_matrix_datasets(file_id, dataset) != OK) {

		fprintf(stderr, "Error creating new disjoint matrix dataset\n");
		ret = NOK;
	}

	H5Fclose(file_id);

	return ret;
}

oknok_t create_disjoint_matrix_datasets(const hid_t file_id,
		const dataset_t *dataset) {

//	if (create_attribute_line_dataset(file_id, dataset) != OK) {
//		return NOK;
//	}

	return create_attribute_column_dataset(file_id, dataset);
}

oknok_t create_attribute_column_dataset(const hid_t file_id,
		const dataset_t *dataset) {

	// Number of longs in a line
	uint32_t n_words = dataset->n_words;

	// Number of observations
	uint32_t n_observations = dataset->n_observations;

	// Number of classes
	uint32_t n_classes = dataset->n_classes;

	// Observations per class
	word_t **observations_per_class = dataset->observations_per_class;

	// Number of observations per class
	uint32_t *n_observations_per_class = dataset->n_observations_per_class;

	uint32_t n_lines = calculate_number_of_lines_of_disjoint_matrix(dataset);

	oknok_t ret = OK;

	// Dataset dimensions
	hsize_t dm_dimensions[2] = { n_lines, n_words };

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
	hsize_t dm_chunk_dimensions[2] = { 1, n_words };

	H5Pset_chunk(dm_property_list_id, 2, dm_chunk_dimensions);

	// Create the dataset
	hid_t dm_dataset_id = H5Dcreate2(file_id, DM_DATASET_ATTRIBUTES_COLUMN,
	H5T_STD_U64LE, dm_dataset_space_id, H5P_DEFAULT, dm_property_list_id,
	H5P_DEFAULT);
	if (dm_dataset_id < 0) {
		fprintf(stderr, "Error creating disjoint matrix dataset\n");
		ret = NOK;
		goto out_dataset_space;
	}

	// Save attributes
	if (write_disjoint_matrix_attributes(dm_dataset_id, dataset->n_attributes,
			n_lines) < 0) {

		fprintf(stderr, "Error saving matrix atributes");
		ret = NOK;
		goto out_dataset;
	}

	// Close resources
	H5Pclose(dm_property_list_id);

	hsize_t mem_dimensions[2] = { 1, n_words };
	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t dm_memory_space_id = H5Screate_simple(2, mem_dimensions, NULL);
	if (dm_memory_space_id < 0) {
		fprintf(stderr, "Error creating disjoint matrix memory space\n");
		ret = NOK;
		goto out_memory_space;
	}

	// Alocate buffer
	word_t *buffer = (word_t*) malloc(sizeof(word_t) * n_words);

	// We will write one line at a time
	hsize_t count[2] = { 1, n_words };
	hsize_t offset[2] = { 0, 0 };

	// Used to print out progress message
	uint32_t next_output = 0;

	// Current line
	for (uint32_t i = 0; i < n_classes - 1; i++) {
		for (uint32_t j = i + 1; j < n_classes; j++) {
			for (uint32_t n_i = i * n_observations;
					n_i < i * n_observations + n_observations_per_class[i];
					n_i++) {
				for (uint32_t n_j = j * n_observations;
						n_j < j * n_observations + n_observations_per_class[j];
						n_j++) {
					for (uint32_t n = 0; n < n_words; n++) {
						buffer[n] = observations_per_class[n_i][n]
								^ observations_per_class[n_j][n];
					}

					// Select hyperslab on file dataset
					H5Sselect_hyperslab(dm_dataset_space_id, H5S_SELECT_SET,
							offset, NULL, count, NULL);

					// Write buffer to dataset
					herr_t err = H5Dwrite(dm_dataset_id, H5T_NATIVE_ULONG,
							dm_memory_space_id, dm_dataset_space_id,
							H5P_DEFAULT, buffer);

					if (err < 0) {
						goto out_free_buffer;
					}

					// Update offset
					offset[0]++;

					if (offset[0] > next_output) {
						fprintf(stdout,
								" - Writing disjoint matrix [1/2]: %0.0f%%  \r",
								((double) offset[0]) / n_lines * 100);
						fflush( stdout);

						next_output += n_lines / 10;
					}
				}
			}
		}
	}

out_free_buffer:
	free(buffer);

out_memory_space:
	H5Sclose(dm_memory_space_id);

out_dataset_space:
	H5Sclose(dm_dataset_space_id);

out_dataset:
	H5Dclose(dm_dataset_id);

	return ret;
}

oknok_t create_attribute_line_dataset(const hid_t file_id,
		const dataset_t *dataset) {

	// Number of attributes
	uint32_t n_attributes = dataset->n_attributes;

	// Number of observations
	uint32_t n_observations = dataset->n_observations;

	// Number of classes
	uint32_t n_classes = dataset->n_classes;

	// Observations per class
	word_t **observations_per_class = dataset->observations_per_class;

	// Number of observations per class
	uint32_t *n_observations_per_class = dataset->n_observations_per_class;

	uint32_t n_lines = calculate_number_of_lines_of_disjoint_matrix(dataset);

	// Number of words in a line
	uint32_t n_words_line = n_lines / WORD_BITS + (n_lines % WORD_BITS != 0);

	oknok_t ret = OK;

	// Dataset dimensions
	hsize_t dm_dimensions[2] = { n_attributes, n_words_line };

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
	hsize_t dm_chunk_dimensions[2] = { 1, n_words_line };

	H5Pset_chunk(dm_property_list_id, 2, dm_chunk_dimensions);

	// Create the dataset
	hid_t dm_dataset_id = H5Dcreate2(file_id, DM_DATASET_ATTRIBUTES_LINE,
	H5T_STD_U64LE, dm_dataset_space_id, H5P_DEFAULT, dm_property_list_id,
	H5P_DEFAULT);
	if (dm_dataset_id < 0) {
		fprintf(stderr, "Error creating disjoint matrix dataset\n");
		ret = NOK;
		goto out_dataset_space;
	}

	// Save attributes
	if (write_disjoint_matrix_attributes(dm_dataset_id, n_attributes, n_lines)
			< 0) {

		fprintf(stderr, "Error saving matrix atributes");
		ret = NOK;
		goto out_dataset;
	}

	// Close resources
	H5Pclose(dm_property_list_id);

	hsize_t mem_dimensions[2] = { 1, n_words_line };
	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t dm_memory_space_id = H5Screate_simple(2, mem_dimensions, NULL);
	if (dm_memory_space_id < 0) {
		fprintf(stderr, "Error creating disjoint matrix memory space\n");
		ret = NOK;
		goto out_memory_space;
	}

	// Alocate buffer
	word_t *buffer = (word_t*) malloc(sizeof(word_t) * n_lines);

	// We will write one line at a time
	hsize_t count[2] = { 1, n_words_line };
	hsize_t offset[2] = { 0, 0 };

	// Used to print out progress message
	uint32_t next_output = 0;

	// Current buffer position
	word_t *current_buffer = buffer;

	for (uint32_t n = 0; n < dataset->n_words; n++) {
		current_buffer = buffer;
		for (uint32_t ci = 0; ci < n_classes - 1; ci++) {
			for (uint32_t cj = ci + 1; cj < n_classes; cj++) {
				for (uint32_t n_i = ci * n_observations;
						n_i < ci * n_observations + n_observations_per_class[ci];
						n_i++) {
					for (uint32_t n_j = cj * n_observations;
							n_j
									< cj * n_observations
											+ n_observations_per_class[cj];
							n_j++, current_buffer++) {

						*current_buffer = observations_per_class[n_i][n]
								^ observations_per_class[n_j][n];
					}
				}
			}
		}

		// The array now has data for (up to) WORD_BITS attributes.
		// We need to extract it and store it in the dataset

		// Number of attributes actually computed. We are working in blocks of
		// WORD_BITS bits, but the last block may be incomplete
		// and only have < WORD_BITS attributes in it
		uint8_t read_attributes = WORD_BITS;
		if (n == dataset->n_words - 1) {
			read_attributes = n_attributes - WORD_BITS * n;
		}

		save_attribute_data(dm_dataset_id, dm_dataset_space_id,
				dm_memory_space_id, offset, count, buffer, n_lines,
				read_attributes);

		if (offset[0] > next_output) {
			fprintf(stdout, " - Writing disjoint matrix [2/2]: %0.0f%%      \r",
					((double) offset[0]) / n_attributes * 100);
			fflush( stdout);

			next_output += n_attributes / 10;
		}
	}

	free(buffer);

out_memory_space:
	H5Sclose(dm_memory_space_id);

out_dataset_space:
	H5Sclose(dm_dataset_space_id);

out_dataset:
	H5Dclose(dm_dataset_id);

	return ret;
}

herr_t save_attribute_data(const hid_t dm_dataset_id,
		const hid_t dm_dataset_space_id, const hid_t dm_memory_space_id,
		hsize_t *offset, const hsize_t *count, const word_t *data,
		const uint32_t n_lines, const uint8_t n_attributes) {

	uint32_t n_words = count[1];

	word_t *buffer = (word_t*) malloc(sizeof(word_t) * n_words);

	// CUrrent line
	uint32_t cl = 0;

	for (uint8_t i = WORD_BITS; i > WORD_BITS - n_attributes; i--) {
		// Reset buffer
		memset(buffer, 0, sizeof(word_t) * n_words);
		cl = 0;

		for (uint32_t n = 0; n < n_words; n++) {

			for (int j = WORD_BITS - 1; j >= 0 && cl < n_lines; j--, cl++) {
				if (AND_MASK_TABLE[i - 1] & data[cl]) {
					buffer[n] |= AND_MASK_TABLE[j];
				}
			}
		}

		// Save to file
		// Select hyperslab on file dataset
		H5Sselect_hyperslab(dm_dataset_space_id, H5S_SELECT_SET, offset, NULL,
				count, NULL);

		// Write buffer to dataset
		H5Dwrite(dm_dataset_id, H5T_NATIVE_ULONG, dm_memory_space_id,
				dm_dataset_space_id, H5P_DEFAULT, buffer);

		// Update offset
		offset[0]++;
	}

	free(buffer);

	return OK;
}

herr_t write_disjoint_matrix_attributes(const hid_t dataset_id,
		const uint32_t n_attributes, const uint32_t n_matrix_lines) {

	herr_t ret = 0;

	ret = hdf5_write_attribute(dataset_id, HDF5_N_ATTRIBUTES_ATTRIBUTE,
	H5T_NATIVE_UINT, &n_attributes);
	if (ret < 0) {
		return ret;
	}

	ret = hdf5_write_attribute(dataset_id, HDF5_N_MATRIX_LINES_ATTRIBUTE,
	H5T_NATIVE_UINT, &n_matrix_lines);

	return ret;
}
