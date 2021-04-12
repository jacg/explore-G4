#include "hdf5_writer.hh"

hdf5_writer::hdf5_writer()
: file_{0}
, group_{0}
, run_table_{0}
, memtype_run_{0}
, irun_{0} {}


void hdf5_writer::open(std::string file_name) {
    file_ = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    group_          = create_group(file_, "/MC");
    run_table_      = create_table(group_, "configuration", memtype_run_      = create_run_type());
}

void hdf5_writer::write_run_info(const char* param_key, const char* param_value) {
    run_info_t run_data;
	set_param(run_data.param_key  , param_key);
	set_param(run_data.param_value, param_value);
    write_table_data((void*) &run_data, run_table_, memtype_run_, irun_);

    irun_++;
}

