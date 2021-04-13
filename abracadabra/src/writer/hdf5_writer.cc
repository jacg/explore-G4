#include "hdf5_writer.hh"

hdf5_writer::hdf5_writer()
: file_{0}
, group_{0}
, run_table_{0}
, hits_table_{0}
, memtype_run_{0}
, memtype_hits_{0}
, irun_{0}
, ihit_{0} {}


void hdf5_writer::open(std::string file_name) {
    file_ = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    group_          = create_group(file_, "/MC");
    run_table_      = create_table(group_, "configuration", memtype_run_  = create_run_type());
    hits_table_     = create_table(group_, "hits"         , memtype_hits_ = create_hit_type());
}

void hdf5_writer::write_run_info(const char* param_key, const char* param_value) {
    run_info_t run_data;
	set_param(run_data.param_key  , param_key);
	set_param(run_data.param_value, param_value);
    write_table_data((void*) &run_data, run_table_, memtype_run_, irun_);

    irun_++;
}


void hdf5_writer::write_hit_info(unsigned int event_id, double x, double y, double z) {
    hit_t hit_data;
	hit_data.event_id = event_id;
	hit_data.x        = x;
	hit_data.y        = y;
	hit_data.z        = z;
    write_table_data((void*) &hit_data, hits_table_, memtype_hits_, ihit_);
    ihit_++;
}

