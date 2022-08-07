/*
 ============================================================================
 Name        : dataset_hdf5_t.h
 Author      : Eduardo Ribeiro
 Description : Datatype representing one hgf5 dataset
 ============================================================================
 */

#ifndef dataset_hdf5_t_H
#define dataset_hdf5_t_H

#include "hdf5.h"

typedef struct dataset_hdf5_t
{
	/**
	 * file_id
	 */
	hid_t file_id;

	/**
	 * dataset_id
	 */
	hid_t dataset_id;

	/**
	 * Dimensions
	 */
	hsize_t dimensions[2];

} dataset_hdf5_t;

#endif // dataset_hdf5_t_H
