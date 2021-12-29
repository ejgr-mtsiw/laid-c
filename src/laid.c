/*
 ============================================================================
 Name        : laid.c
 Author      : Eduardo Ribeiro
 Description : Basic implementation of the LAID algorithm in C + HDF5
 ============================================================================
 */

#include <clargs.h>
#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"
#include <limits.h>

#define VALID_DIMENSIONS 0
#define INVALID_DIMENSIONS 1

/**
 * How many bits in a long?
 */
size_t get_number_of_bits_in_a_long() {
	return sizeof(unsigned long int) * CHAR_BIT;
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
	 * Buffer to store the dataset
	 */
	unsigned long int *buffer = NULL;

	// Parse command line arguments
	if (read_args(argc, argv, &args) == READ_CL_ARGS_NOK) {
		return EXIT_FAILURE;
	}

	//! TODO: Open the data file

	//! TODO: Read the dataset

	//! TODO: Remove duplicated lines

	//! TODO: Fill JNSQ table for inconsistent data entries

	//! TODO: Apply LAD

	fprintf(stdout, "All done!\n");

	return EXIT_SUCCESS;
}
