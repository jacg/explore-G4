#ifndef HDF5_FUNCTIONS_H
#define HDF5_FUNCTIONS_H

#include <hdf5.h>
#include <iostream>

#define CONFLEN 300
#define STRLEN 100

typedef struct{
	char param_key[CONFLEN];
	char param_value[CONFLEN];
} run_info_t;

hsize_t create_run_type();

hid_t create_table(hid_t group, std::string& table_name, hsize_t memtype);
hid_t create_group(hid_t file, std::string& group_name);

void write_run(run_info_t* run_data, hid_t dataset, hid_t memtype, hsize_t counter);


#endif
