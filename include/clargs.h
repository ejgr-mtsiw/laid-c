/*
 ============================================================================
 Name        : clargs.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage command line arguments
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef __CL_ARGS_INCLUDE__
#define __CL_ARGS_INCLUDE__

/**
 * Do not edit
 */
#define READ_CL_ARGS_OK 0
#define READ_CL_ARGS_NOK 1

/**
 * Structure to store command line options
 */
typedef struct {
	/**
	 * The name of the dataset file
	 */
	char *filename;

	/**
	 * The dataset identifier
	 */
	char *datasetname;
} clargs;

int read_args(int argc, char **argv, clargs *args) {
	int c = 0;

	opterr = 0;

	// Set defaults
	args->filename = NULL;
	args->datasetname = NULL;

	while ((c = getopt(argc, argv, "f:d:")) != -1) {
		switch (c) {
		case 'f':
			args->filename = optarg;
			break;
		case 'd':
			args->datasetname = optarg;
			break;
		case '?':
			if (optopt == 'f') {
				fprintf(stderr, "Must set output filename (-f).\n");
				return READ_CL_ARGS_NOK;
			}

			if (optopt == 'd') {
				fprintf(stderr, "Must set dataset identifier (-d).\n");
				return READ_CL_ARGS_NOK;
			}
			break;

		default:
			abort();
		}
	}

	if (args->filename == NULL || args->datasetname == NULL) {
		fprintf(stdout, "Usage:\n %s -f <filename> -d <dataset>\n", argv[0]);
		return READ_CL_ARGS_NOK;
	}

	return READ_CL_ARGS_OK;
}

#endif
