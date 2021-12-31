/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Basic implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

#include "bit_utils.h"
#include "clargs.h"
#include "dataset.h"
#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"
#include <math.h>
#include <string.h>

void print_line(const unsigned long n_cols, const unsigned long n_attributes, const int n_bits_for_classes,
		const unsigned long *buffer) {

	unsigned long column = 0;
	int classe_bits = 0;
	int classe = 0;

	for (hsize_t i = 0; i < n_cols; i++) {
		for (unsigned int j = 0; j < LONG_BITS; j++) {
			if (column < n_attributes) {
				if (j % 8 == 0) {
					printf(" ");
				}

				printf("%d", CHECK_BIT(buffer[i], LONG_BITS - 1 - j));
			} else {
				if (classe_bits < n_bits_for_classes) {
					classe <<= 1;
					classe += CHECK_BIT(buffer[i], LONG_BITS - 1 - j);
					classe_bits++;
				} else {
					// done!
					break;
				}
			}
			column++;
		}
	}

	printf(" | ");
	printf("%d\n", classe);
}

/**
 * Checks if the arrays have the same attributes
 */
int has_same_attributes(unsigned long *a, unsigned long *b, unsigned long n_attributes) {

	// Number of longs filled only with attributes
	int n_longs = n_attributes / LONG_BITS;

	int i = 0;
	// We only have attributes here so we can fast check
	for (i = 0; i < n_longs - 1; i++) {
		if (a[i] != b[i]) {
			return 0;
		}
	}

	// Check the remaining attributes one by one
	int remaining_attributes = n_attributes - (n_longs * LONG_BITS);

	for (i = 1; i <= remaining_attributes; i++) {
		if (CHECK_BIT(a[n_longs], LONG_BITS-i) != CHECK_BIT(b[n_longs], LONG_BITS-i)) {
			return 0;
		}
	}

	// Every attribute is the same
	return 1;
}

/**
 * Checks if the arrays have the same class
 */
int has_same_class(const unsigned long *a, const unsigned long *b, const unsigned long n_attributes,
		int n_bits_for_classes) {
	// Number of longs filled only with attributes
	int n_longs = n_attributes / LONG_BITS;

	// Check how many attributes remain
	int remaining_attributes = n_attributes - (n_longs * LONG_BITS);

	// CLass starts on this bit
	int class_start = LONG_BITS - remaining_attributes;

	// Check the bits after the attributes
	for (int i = 1; i <= n_bits_for_classes; i++) {
		if (CHECK_BIT(a[n_longs], class_start - i) != CHECK_BIT(b[n_longs], class_start - i)) {
			// Distinct classes
			return 0;
		}
	}

	// They have the same class
	return 1;
}

/**
 *
 */
int main(int argc, char **argv) {

	/**
	 * Command line arguments set by the user
	 */
	clargs args;

	/**
	 * File identifier
	 */
	hid_t file_id = 0;

	/**
	 * Dataset identifier
	 */
	hid_t dataset_id = 0;

	/**
	 * Dataset dataspace identifier
	 */
	hid_t dataset_space_id = 0;

	/**
	 * Dataset creation property list identifier
	 */
	hid_t property_list_id = 0;

	/**
	 * In memory dataspace identifier
	 */
	hid_t memory_space_id = 0;

	/**
	 * Store the result of operations
	 */
	//herr_t status = 0;
	/**
	 * Dataset dimensions
	 */
	hsize_t dataset_dimensions[2] = { 0, 0 };

	/**
	 * Chunk dimensions
	 */
	hsize_t chunk_dimensions[2] = { 0, 0 };

	/**
	 * Buffer to store one line/chunk of data
	 */
	unsigned long *buffer = NULL;

	/**
	 * Full dataset in memory
	 */
	unsigned long *full_dataset = NULL;

	// Parse command line arguments
	if (read_args(argc, argv, &args) == READ_CL_ARGS_NOK) {
		return EXIT_FAILURE;
	}

	//Open the data file
	file_id = H5Fopen(args.filename, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 1) {
		// Error creating file
		fprintf(stdout, "Error opening file %s\n", args.filename);
		return EXIT_FAILURE;
	}
	fprintf(stdout, " - Dataset file opened.\n");

	dataset_id = H5Dopen2(file_id, args.datasetname, H5P_DEFAULT);
	if (dataset_id < 1) {
		// Error creating file
		fprintf(stdout, "Dataset %s not found!\n", args.datasetname);
		return EXIT_FAILURE;
	}
	fprintf(stdout, " - Dataset opened.\n");

	// Get filespace handle first.
	dataset_space_id = H5Dget_space(dataset_id);

	// Get dataset rank and dimension.
	int rank = H5Sget_simple_extent_ndims(dataset_space_id);
	H5Sget_simple_extent_dims(dataset_space_id, dataset_dimensions, NULL);
	printf("dataset rank %d, dimensions %lu x %lu\n", rank, (unsigned long) (dataset_dimensions[0]),
			(unsigned long) (dataset_dimensions[1]));

	// Get attributes
	int n_classes = 0;
	unsigned long n_observations = 0, n_attributes = 0;

	read_attribute(dataset_id, "n_classes", H5T_NATIVE_INT, &n_classes);
	printf("The value of the attribute \"n_classes\" is %d \n", n_classes);

	read_attribute(dataset_id, "n_observations", H5T_NATIVE_ULONG, &n_observations);
	printf("The value of the attribute \"n_observations\" is %lu \n", n_observations);

	read_attribute(dataset_id, "n_attributes", H5T_NATIVE_ULONG, &n_attributes);
	printf("The value of the attribute \"n_attributes\" is %lu \n", n_attributes);

	//! TODO: Read the dataset
	// Number of bits for classes
	int n_bits_for_classes = (int) ceil(log2(n_classes));

	// Maximum number of bits for possibles jnsqs
	int n_bits_for_jnsqs = n_bits_for_classes;

	unsigned long total_bits = n_attributes + n_bits_for_classes + n_bits_for_jnsqs;
	fprintf(stdout, "total_bits = %lu\n", total_bits);

	unsigned long n_cols = total_bits / LONG_BITS + (total_bits % LONG_BITS != 0);
	fprintf(stdout, "n_cols = %lu\n", n_cols);

	// Allocate main buffer
	full_dataset = (unsigned long*) malloc(sizeof(unsigned long) * n_observations * n_cols);

	// Get creation properties list.
	property_list_id = H5Dget_create_plist(dataset_id);

	if (H5D_CHUNKED == H5Pget_layout(property_list_id)) {

		// Get chunking information: rank and dimensions
		int rank_chunk = H5Pget_chunk(property_list_id, 2, chunk_dimensions);
		printf("chunk rank %d, dimensions %lu x %lu\n", rank_chunk, (unsigned long) (chunk_dimensions[0]),
				(unsigned long) (chunk_dimensions[1]));

		// Allocate chunk buffer
		buffer = (unsigned long*) malloc(sizeof(unsigned long) * chunk_dimensions[1]);

		/*
		 * Define the memory space to read a chunk.
		 */
		memory_space_id = H5Screate_simple(rank_chunk, chunk_dimensions, NULL);

		/*
		 * Define chunk in the file (hyperslab) to read.
		 */
		hsize_t offset[2] = { 0, 0 };
		hsize_t count[2] = { chunk_dimensions[0], chunk_dimensions[1] };

		unsigned long *first = NULL;
		unsigned long *last = NULL;

		// How many entries have the same attributes but different classes?
		int inconsistency_level = 0;

		for (hsize_t line = 0; line < dataset_dimensions[0]; line++) {

			offset[0] = line;

			H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset, NULL, count, NULL);

			// Read chunk back
			H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id, dataset_space_id, H5P_DEFAULT, buffer);

			// Print line
			//print_line(n_cols, n_attributes, n_bits_for_classes, buffer);

			// Store in main dataset
			int found = 0;
			if (first != NULL) {
				for (unsigned long *current = first; current <= last; current += n_cols) {
					if (has_same_attributes(buffer, current, n_attributes)) {
						// Check class
						if (has_same_class(buffer, current, n_attributes, n_bits_for_classes)) {
							// It's a duplicate
							found = true;
							fprintf(stdout, "Duplicate!\n");
							print_line(chunk_dimensions[1], n_attributes, n_bits_for_classes, buffer);
							print_line(chunk_dimensions[1], n_attributes, n_bits_for_classes, current);
							break;
						} else {
							// It's an inconsistent observation
							inconsistency_level++;
							fprintf(stdout, "Inconsistency!\n");
							print_line(chunk_dimensions[1], n_attributes, n_bits_for_classes, buffer);
							print_line(chunk_dimensions[1], n_attributes, n_bits_for_classes, current);
						}
					}
				}
			}

			if (!found) {
				if (first == NULL) {
					first = full_dataset;
				}

				if (last == NULL) {
					last = full_dataset;
				} else {
					last += n_cols;
				}

				//TODO:Add jnsq

				memcpy(last, buffer, chunk_dimensions[1] * sizeof(unsigned long));
			}
		}

		/*
		 for (unsigned long i = 0; i < n_observations; i++) {
		 print_line(chunk_dimensions[1], n_attributes, n_bits_for_classes, full_dataset + i * n_cols);
		 }
		 */

		/*
		 * Close/release resources.
		 */
		free(buffer);
		free(full_dataset);
		H5Sclose(memory_space_id);
	}

	//! TODO: Remove duplicated lines

	//! TODO: Fill JNSQ table for inconsistent data entries

	//! TODO: Apply LAD

	H5Sclose(dataset_space_id);
	H5Dclose(dataset_id);
	H5Fclose(file_id);

	fprintf(stdout, "All done!\n");

	return EXIT_SUCCESS;
}
