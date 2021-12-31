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
#include "dataset_line.h"
#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"
#include <math.h>

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

	/**
	 * Number of classes in the dataset
	 */
	int n_classes = 0;

	/**
	 * Number of observations (lines) in the dataset
	 */
	unsigned long n_observations = 0;

	/**
	 * Number of distinct observations (non duplicated)
	 */
	unsigned long n_distinct_observations = 0;

	/**
	 * Number of atttributes (columns) in the dataset
	 */
	unsigned long n_attributes = 0;

	/**
	 * Number of bits for classes
	 */
	unsigned int n_bits_for_classes = 0;

	/**
	 * Number of bits for jnsq's
	 */
	unsigned int n_bits_for_jnsqs = 0;

	/**
	 * Number of duplicated observations
	 */
	unsigned long n_duplicates = 0;

	/**
	 * Number of inconsistent rows in the dataser
	 */
	unsigned long n_inconsistencies = 0;

	/**
	 * Maximum number of inconsistencies
	 */
	unsigned long max_inconsistencies = 0;

	/**
	 *
	 */
	//unsigned long total_bits = n_attributes + n_bits_for_jnsqs;
	//fprintf(stdout, "total_bits = %lu\n", total_bits);
	//unsigned long n_longs = total_bits / LONG_BITS + (total_bits % LONG_BITS != 0);
	//unsigned long n_longs = dataset_dimensions[1];
	// Array to store dataset lines info
	dataset_line *dataset_lines = NULL;

	/**
	 * First dataset line
	 */
	dataset_line *first = NULL;

	/**
	 * Last dataset line
	 */
	dataset_line *last = NULL;

	/**
	 * Buffer for a new line
	 */
	dataset_line new_line;

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

	// Get dataset dimensions.
	H5Sget_simple_extent_dims(dataset_space_id, dataset_dimensions, NULL);
	fprintf(stdout, "dataset dimensions %lu x %lu\n", (unsigned long) (dataset_dimensions[0]),
			(unsigned long) (dataset_dimensions[1]));

	// Get attributes
	read_attribute(dataset_id, "n_classes", H5T_NATIVE_INT, &n_classes);
	printf("The value of the attribute \"n_classes\" is %d \n", n_classes);

	read_attribute(dataset_id, "n_observations", H5T_NATIVE_ULONG, &n_observations);
	printf("The value of the attribute \"n_observations\" is %lu \n", n_observations);

	read_attribute(dataset_id, "n_attributes", H5T_NATIVE_ULONG, &n_attributes);
	printf("The value of the attribute \"n_attributes\" is %lu \n", n_attributes);

	// Number of bits for classes
	n_bits_for_classes = (unsigned int) log2(n_classes) + 1;

	// Maximum number of bits for possibles jnsqs
	n_bits_for_jnsqs = n_bits_for_classes;

	//unsigned long total_bits = n_attributes + n_bits_for_jnsqs;
	//fprintf(stdout, "total_bits = %lu\n", total_bits);
	//unsigned long n_longs = total_bits / LONG_BITS + (total_bits % LONG_BITS != 0);
	//unsigned long n_longs = dataset_dimensions[1];
	//fprintf(stdout, "n_longs = %lu\n", n_longs);

	// Allocate main buffer
	//full_dataset = (unsigned long*) malloc(sizeof(unsigned long) * n_observations * n_longs);
	full_dataset = (unsigned long*) malloc(sizeof(unsigned long) * n_observations * dataset_dimensions[1]);

	// Allocate dataset_lines array
	dataset_lines = (dataset_line*) malloc(sizeof(dataset_line) * n_observations);

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

		for (hsize_t line = 0; line < n_observations; line++) {

			offset[0] = line;

			H5Sselect_hyperslab(dataset_space_id, H5S_SELECT_SET, offset, NULL, count, NULL);

			// Read chunk back
			H5Dread(dataset_id, H5T_NATIVE_ULONG, memory_space_id, dataset_space_id, H5P_DEFAULT, buffer);

			new_line.inconsistency = 0;
			new_line.class_id = get_class(buffer, n_attributes, n_bits_for_classes);
			new_line.data = buffer;

			// Print line
			//print_line(&new_line, n_attributes);

			// Check for duplicates and inconsistencies
			int found = 0;
			if (first != NULL) {
				for (dataset_line *current = first; current <= last; current++) {
					if (has_same_attributes(&new_line, current, n_attributes)) {
						// Check class
						if (has_same_class(&new_line, current)) {
							// It's a duplicate
							found = true;
							n_duplicates++;

							fprintf(stdout, "Duplicate!\n");
							print_line(&new_line, n_attributes);
							print_line(current, n_attributes);
							break;
						} else {
							// It's an inconsistent observation
							new_line.inconsistency++;

							// Update global counter
							n_inconsistencies++;

							fprintf(stdout, "Inconsistency!\n");
							print_line(&new_line, n_attributes);
							print_line(current, n_attributes);
						}
					}
				}
			}

			if (!found) {
				// It's either a new entry or an inconsistent one
				// but must be added to the list

				if (first == NULL) {
					first = dataset_lines;
				}

				if (last == NULL) {
					last = dataset_lines;
					last->data = full_dataset;
				} else {
					unsigned long *last_data = last->data;
					last++;
					last->data = last_data + dataset_dimensions[1];
				}

				dataset_line_copy(last, &new_line, n_attributes);

				if (new_line.inconsistency > max_inconsistencies) {
					max_inconsistencies = new_line.inconsistency;
				}
			}
		}

		n_distinct_observations = n_observations - n_duplicates;

		// Set jnsq's
		if (max_inconsistencies == 0) {
			// No inconsistencies!
			n_bits_for_jnsqs = 0;
		} else {
			// Calculate update number of bits needed to store jnsq
			n_bits_for_jnsqs = log2(max_inconsistencies) + 1;

			// Update all lines
			for (dataset_line *line = first; line <= last; line++) {
				set_jnsq(line, n_attributes, n_bits_for_jnsqs);
			}

			// Update n_attributes
			n_attributes += n_bits_for_jnsqs;
		}

		fprintf(stdout, "Found %lu observations, %lu duplicates.\n", n_observations, n_duplicates);

		fprintf(stdout, "Found %lu inconsistencies, max inconsistencies: %lu.\n", n_inconsistencies,
				max_inconsistencies);

		for (unsigned long i = 0; i < n_distinct_observations; i++) {
			print_line(&dataset_lines[i], n_attributes);

			if (i == 9 && n_distinct_observations > 20) {
				// Too many lines, jump to end
				printf(" ... \n");
				i = n_distinct_observations - 11;
			}
		}

		/*
		 * Close/release resources.
		 */
		free(buffer);
		free(full_dataset);
		free(dataset_lines);

		H5Sclose(memory_space_id);
	}

	//! TODO: Apply LAD

	H5Sclose(dataset_space_id);
	H5Dclose(dataset_id);
	H5Fclose(file_id);

	fprintf(stdout, "All done!\n");

	return EXIT_SUCCESS;
}
