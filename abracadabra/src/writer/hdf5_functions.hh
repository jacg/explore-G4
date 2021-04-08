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

typedef struct {
    int32_t event_id;
    int particle_id;
    char particle_name[STRLEN];
    char primary;
    int mother_id;
    float initial_x;
    float initial_y;
    float initial_z;
    float initial_t;
    float final_x;
    float final_y;
    float final_z;
    float final_t;
    char initial_volume[STRLEN];
    char final_volume  [STRLEN];
    float initial_momentum_x;
    float initial_momentum_y;
    float initial_momentum_z;
    float final_momentum_x;
    float final_momentum_y;
    float final_momentum_z;
    float kin_energy;
    float length;
    char creator_proc[STRLEN];
	char final_proc  [STRLEN];
} particle_t;

hsize_t create_run_type();
hsize_t create_particle_type();

hid_t create_table(hid_t group, std::string& table_name, hsize_t memtype);
hid_t create_group(hid_t file, std::string& group_name);

void write_table_data(void* data, hid_t dataset, hid_t memtype, hsize_t counter);

#endif
