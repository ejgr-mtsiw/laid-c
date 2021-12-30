/*
 ============================================================================
 Name        : clargs.h
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage command line arguments
 ============================================================================
 */

#ifndef CL_ARGS_H
#define CL_ARGS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Do not edit
 */
#define READ_CL_ARGS_OK 0
#define READ_CL_ARGS_NOK 1

/**
 * Structure to store command line options
 */
typedef struct clargs {
	/**
	 * The name of the dataset file
	 */
	char *filename;

	/**
	 * The dataset identifier
	 */
	char *datasetname;
} clargs;

/**
 *
 */
int read_args(int argc, char **argv, clargs *args);

#endif
