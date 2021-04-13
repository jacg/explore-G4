#ifndef HDF5_FUNCTIONS_H
#define HDF5_FUNCTIONS_H

#include <hdf5.h>
#include <iostream>

#define CONFLEN 300
#define STRLEN 100

typedef struct {
	char param_key  [CONFLEN];
	char param_value[CONFLEN];
} run_info_t;

typedef struct{
	unsigned int event_id;
	double x;
	double y;
	double z;
} hit_t;

hsize_t create_run_type();
hsize_t create_hit_type();

hid_t create_table(hid_t group, std::string const table_name, hsize_t memtype);
hid_t create_group(hid_t file , std::string const group_name);

void write_table_data(void* data, hid_t dataset, hid_t memtype, hsize_t counter);
void set_param(char * const to, char const * const from);
void set_string(char * const to, std::string const& from);

#endif
