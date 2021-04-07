#include "hdf5_writer.hh"

#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <vector>

#include <stdint.h>
#include <iostream>


hdf5_writer::hdf5_writer():
  file_(0), irun_(0) {}


void hdf5_writer::open(std::string file_name){
  file_ = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  std::string group_name = "/MC";
  group_ = create_group(file_, group_name);

  std::string run_table_name = "configuration";
  memtype_run_ = create_run_type();
  run_table_   = create_table(group_, run_table_name, memtype_run_);
}

void hdf5_writer::close(){
  H5Fclose(file_);
}

void hdf5_writer::write_run_info(const char* param_key, const char* param_value){
  run_info_t run_data;
  memset(run_data.param_key,   0, CONFLEN);
  memset(run_data.param_value, 0, CONFLEN);
  strcpy(run_data.param_key, param_key);
  strcpy(run_data.param_value, param_value);
  write_run(&run_data, run_table_, memtype_run_, irun_);

  irun_++;
}
