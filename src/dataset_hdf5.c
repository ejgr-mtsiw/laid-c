/*
 ============================================================================
 Name        : dataset_hdf5.c
 Author      : Eduardo Ribeiro
 Description : Structures and functions to manage HDF5 datasets
 ============================================================================
 */

#include "dataset_hdf5.h"

#include "types/dataset_t.h"
#include "types/oknok_t.h"
#include "types/word_t.h"

#include "hdf5.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

oknok_t hdf5_open_dataset(const char* filename, const char* datasetname,
						  dataset_hdf5_t* dataset)
{
	// Setup file access template
	hid_t acc_tpl = H5Pcreate(H5P_FILE_ACCESS);
	assert(acc_tpl != NOK);

	// Open the file
	hid_t f_id = H5Fopen(filename, H5F_ACC_RDWR, acc_tpl);
	assert(f_id != NOK);

	// Release file-access template
	herr_t ret = H5Pclose(acc_tpl);
	assert(ret != NOK);

	// Open the dataset
	hid_t dset_id = H5Dopen(f_id, datasetname, H5P_DEFAULT);
	assert(dset_id != NOK);

	dataset->file_id	= f_id;
	dataset->dataset_id = dset_id;
	hdf5_get_dataset_dimensions(dset_id, dataset->dimensions);

	return OK;
}

hid_t hdf5_create_dataset(const hid_t file_id, const char* name,
						  const uint32_t n_lines, const uint32_t n_words,
						  const hid_t datatype)
{
	// Dataset dimensions
	hsize_t dimensions[2] = { n_lines, n_words };

	hid_t filespace_id = H5Screate_simple(2, dimensions, NULL);
	assert(filespace_id != NOK);

	// Create a dataset creation property list
	hid_t dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
	assert(dcpl_id != NOK);

	// Create a dataset access property list
	hid_t dapl_id = H5Pcreate(H5P_DATASET_ACCESS);
	assert(dapl_id != NOK);

	// Create the dataset
	hid_t dset_id = H5Dcreate(file_id, name, datatype, filespace_id,
							  H5P_DEFAULT, dcpl_id, dapl_id);
	assert(dset_id != NOK);

	// Close resources
	H5Pclose(dapl_id);
	H5Pclose(dcpl_id);
	H5Sclose(filespace_id);

	return dset_id;
}

bool hdf5_dataset_exists(const hid_t file_id, const char* datasetname)
{
	return (H5Lexists(file_id, datasetname, H5P_DEFAULT) > 0);
}

bool hdf5_file_has_dataset(const char* filename, const char* datasetname)
{
	// Open the data file
	hid_t file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file_id < 1)
	{
		// Error creating file
		fprintf(stderr, "Error opening file %s\n", filename);
		return false;
	}

	bool exists = hdf5_dataset_exists(file_id, datasetname);

	H5Fclose(file_id);

	return exists;
}

oknok_t hdf5_read_dataset_attributes(hid_t dataset_id, dataset_t* dataset)
{
	uint32_t n_classes = 0;
	hdf5_read_attribute(dataset_id, N_CLASSES_ATTR, H5T_NATIVE_UINT32,
						&n_classes);

	if (n_classes < 2)
	{
		fprintf(stderr, "Dataset must have at least 2 classes\n");
		return NOK;
	}

	// Number of observations (lines) in the dataset
	uint32_t n_observations = 0;
	hdf5_read_attribute(dataset_id, N_OBSERVATIONS_ATTR, H5T_NATIVE_UINT32,
						&n_observations);

	if (n_observations < 2)
	{
		fprintf(stderr, "Dataset must have at least 2 observations\n");
		return NOK;
	}

	// Number of attributes in the dataset
	uint32_t n_attributes = 0;
	hdf5_read_attribute(dataset_id, N_ATTRIBUTES_ATTR, H5T_NATIVE_UINT32,
						&n_attributes);

	if (n_attributes < 1)
	{
		fprintf(stderr, "Dataset must have at least 1 attribute\n");
		return NOK;
	}

	// Store data
	dataset->n_attributes	  = n_attributes;
	dataset->n_bits_for_class = (uint8_t) ceil(log2(n_classes));
	dataset->n_bits_for_jnsqs = 0;
	dataset->n_classes		  = n_classes;
	dataset->n_observations	  = n_observations;

	uint32_t total_bits = dataset->n_attributes + dataset->n_bits_for_class;
	uint32_t n_words = total_bits / WORD_BITS + (total_bits % WORD_BITS != 0);

	dataset->n_words = n_words;

	return OK;
}

oknok_t hdf5_read_attribute(hid_t dataset_id, const char* attribute,
							hid_t datatype, void* value)
{
	herr_t status = H5Aexists(dataset_id, attribute);
	if (status < 0)
	{
		// Error reading attribute
		fprintf(stderr, "Error reading attribute %s", attribute);
		return NOK;
	}

	if (status == 0)
	{
		// Attribute does not exist
		fprintf(stderr, "Attribute %s does not exist", attribute);
		return NOK;
	}

	// Open the attribute
	hid_t attr = H5Aopen(dataset_id, attribute, H5P_DEFAULT);
	if (attr < 0)
	{
		fprintf(stderr, "Error opening the attribute %s", attribute);
		return NOK;
	}

	// Read the attribute value
	status = H5Aread(attr, datatype, value);
	if (status < 0)
	{
		fprintf(stderr, "Error reading attribute %s", attribute);
		return NOK;
	}

	// Close the attribute
	status = H5Aclose(attr);
	if (status < 0)
	{
		fprintf(stderr, "Error closing the attribute %s", attribute);
		return NOK;
	}

	return OK;
}

oknok_t hdf5_read_dataset_data(hid_t dataset_id, word_t* data)
{
	// Fill dataset from hdf5 file
	herr_t status = H5Dread(dataset_id, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL,
							H5P_DEFAULT, data);

	if (status < 0)
	{
		fprintf(stderr, "Error reading the dataset data\n");

		data = NULL;
		return NOK;
	}

	return OK;
}

oknok_t hdf5_read_line(const dataset_hdf5_t* dataset, const uint32_t index,
					   const uint32_t n_words, word_t* line)
{
	return hdf5_read_lines(dataset, index, n_words, 1, line);
}

oknok_t hdf5_read_lines(const dataset_hdf5_t* dataset, const uint32_t index,
						const uint32_t n_words, const uint32_t n_lines,
						word_t* lines)
{
	// Setup offset
	hsize_t offset[2] = { index, 0 };

	// Setup count
	hsize_t count[2] = { n_lines, n_words };

	const hsize_t dimensions[2] = { n_lines, n_words };

	// Create a memory dataspace to indicate the size of our buffer/chunk
	hid_t memspace_id = H5Screate_simple(2, dimensions, NULL);

	// Setup line dataspace
	hid_t dataspace_id = H5Dget_space(dataset->dataset_id);

	// Select hyperslab on file dataset
	H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, offset, NULL, count,
						NULL);

	// Read line from dataset
	H5Dread(dataset->dataset_id, H5T_NATIVE_UINT64, memspace_id, dataspace_id,
			H5P_DEFAULT, lines);

	H5Sclose(dataspace_id);
	H5Sclose(memspace_id);

	return OK;
}

oknok_t hdf5_write_attribute(hid_t dataset_id, const char* attribute,
							 hid_t datatype, const void* value)
{
	hid_t attr_dataspace = H5Screate(H5S_SCALAR);
	hid_t attr = H5Acreate(dataset_id, attribute, datatype, attr_dataspace,
						   H5P_DEFAULT, H5P_DEFAULT);
	if (attr < 0)
	{
		fprintf(stderr, "Error cretaing attribute %s.\n", attribute);
		return NOK;
	}

	// Write the attribute to the dataset
	herr_t status = H5Awrite(attr, datatype, value);
	if (status < 0)
	{
		fprintf(stderr, "Error writing attribute %s.\n", attribute);
		return NOK;
	}

	// Close the attribute.
	status = H5Aclose(attr);
	if (status < 0)
	{
		fprintf(stderr, "Error closing attribute %s.\n", attribute);
		return NOK;
	}

	// Close the dataspace.
	status = H5Sclose(attr_dataspace);
	if (status < 0)
	{
		fprintf(stderr, "Error closing attribute %s datatspace.\n", attribute);
		return NOK;
	}

	return OK;
}

void hdf5_get_dataset_dimensions(hid_t dataset_id, hsize_t* dataset_dimensions)
{
	// Get filespace handle first.
	hid_t dataset_space_id = H5Dget_space(dataset_id);

	// Get dataset dimensions.
	H5Sget_simple_extent_dims(dataset_space_id, dataset_dimensions, NULL);

	// Close dataspace
	H5Sclose(dataset_space_id);
}

void hdf5_close_dataset(dataset_hdf5_t* dataset)
{
	H5Dclose(dataset->dataset_id);
	H5Fclose(dataset->file_id);
}

oknok_t hdf5_write_n_lines(const hid_t dset_id, const uint32_t start,
						   const uint32_t n_lines, const uint32_t n_words,
						   const hid_t datatype, const void* buffer)
{
	/**
	 * If we don't have anything to write, return here
	 */
	if (n_lines == 0 || n_words == 0)
	{
		return OK;
	}

	// We will write n_lines_out lines at a time
	hsize_t count[2]  = { n_lines, n_words };
	hsize_t offset[2] = { start, 0 };

	return hdf5_write_to_dataset(dset_id, offset, count, datatype, buffer);
}

oknok_t hdf5_write_to_dataset(const hid_t dset_id, const hsize_t offset[2],
							  const hsize_t count[2], const hid_t datatype,
							  const void* buffer)
{
	/**
	 * If we don't have anything to write, return here
	 */
	if (count[0] == 0 || count[1] == 0)
	{
		return OK;
	}

	/**
	 * Setup dataspace
	 * If writing to a portion of a dataset in a loop, be sure
	 * to close the dataspace with each iteration, as this
	 * can cause a large temporary "memory leak".
	 * "Achieving High Performance I/O with HDF5"
	 */
	hid_t filespace_id = H5Dget_space(dset_id);
	assert(filespace_id != NOK);

	// Select hyperslab on file dataset
	herr_t err = H5Sselect_hyperslab(filespace_id, H5S_SELECT_SET, offset, NULL,
									 count, NULL);
	assert(err != NOK);

	// Create a memory dataspace to indicate the size of our buffer
	//	hsize_t mem_dimensions[2] = count;
	hid_t memspace_id = H5Screate_simple(2, count, NULL);
	assert(memspace_id != NOK);

	// set up the collective transfer properties list
	hid_t xfer_plist = H5Pcreate(H5P_DATASET_XFER);
	assert(xfer_plist != NOK);

	/**
	 * H5FD_MPIO_COLLECTIVE transfer mode is not favourable:
	 * https://docs.hdfgroup.org/hdf5/rfc/coll_ind_dd6.pdf
	 */
	/*
	err = H5Pset_dxpl_mpio(xfer_plist, H5FD_MPIO_COLLECTIVE);
	assert(err != NOK);
	*/

	// Write buffer to dataset
	err = H5Dwrite(dset_id, datatype, memspace_id, filespace_id, xfer_plist,
				   buffer);
	assert(err != NOK);

	H5Pclose(xfer_plist);
	H5Sclose(memspace_id);
	H5Sclose(filespace_id);

	return OK;
}
