#include "hdf5_writer.hh"
#include <highfive/H5DataSpace.hpp>

#include <cstring>
#include <iostream>

#define CONFLEN 300

hdf5_writer::hdf5_writer(std::string fname)
: filename{fname}
, runinfo_index{0}
, hit_index{0} {}


HighFive::CompoundType create_hit_type() {
    return {{"event_id", HighFive::AtomicType<unsigned int>{}},
            {"x", HighFive::AtomicType<double>{}},
            {"y", HighFive::AtomicType<double>{}},
            {"z", HighFive::AtomicType<double>{}}};
}
HIGHFIVE_REGISTER_TYPE(hit_t, create_hit_type)


HighFive::CompoundType create_runinfo_type() {
    return {{"param_key"  , HighFive::AtomicType<char[CONFLEN]>{}},
            {"param_value", HighFive::AtomicType<char[CONFLEN]>{}}};
}

HIGHFIVE_REGISTER_TYPE(run_info_t, create_runinfo_type)

void set_string_param(char * to, const char * from, unsigned int max_len) {
  memset(to, 0, max_len);
  strcpy(to, from);
}

void hdf5_writer::open() {
    HighFive::File file = HighFive::File{filename, HighFive::File::ReadWrite | HighFive::File::Create | HighFive::File::Truncate};
    HighFive::Group group = file.createGroup("MC");

    // To create a table than can be resized it has be of UNLIMITED dimension
    // and requires chunking of the data
    HighFive::DataSpace dataspace = HighFive::DataSpace({0}, {HighFive::DataSpace::UNLIMITED});
    HighFive::DataSetCreateProps props;
    props.add(HighFive::Chunking(std::vector<hsize_t>{32768}));

    HighFive::DataSet hits_table    = group.createDataSet("hits"         , dataspace, create_hit_type()    , props);
    HighFive::DataSet runinfo_table = group.createDataSet("configuration", dataspace, create_runinfo_type(), props);
}

void hdf5_writer::write_run_info(const char* param_key, const char* param_value) {
    run_info_t runinfo;
    set_string_param(runinfo.param_key  , param_key  , CONFLEN);
    set_string_param(runinfo.param_value, param_value, CONFLEN);
    std::vector<run_info_t> data;
    data.push_back(runinfo);

    unsigned int n_elements = data.size();

    // Get the table from the file
    HighFive::File    file       = HighFive::File{filename, HighFive::File::ReadWrite};
    HighFive::Group   group      = file.getGroup("MC");
    HighFive::DataSet hits_table = group.getDataSet("configuration");

    // Create extra space in the table and append the new data
    hits_table.resize({runinfo_index + n_elements});
    hits_table.select({runinfo_index}, {n_elements}).write(data);

    runinfo_index += n_elements;
}


void hdf5_writer::write_hit_info(unsigned int event_id, double x, double y, double z) {
    // Create hit_t objects with the data
    std::vector<hit_t> data;
    data.push_back({event_id, x, y, z});

    unsigned int n_elements = data.size();

    // Get the table from the file
    HighFive::File    file       = HighFive::File{filename, HighFive::File::ReadWrite};
    HighFive::Group   group      = file.getGroup("MC");
    HighFive::DataSet hits_table = group.getDataSet("hits");

    // Create extra space in the table and append the new data
    hits_table.resize({hit_index + n_elements});
    hits_table.select({hit_index}, {n_elements}).write(data);

    hit_index += n_elements;
}


void hdf5_writer::read_hit_info(std::vector<hit_t>& hits) {
    // Get the table from the file
    HighFive::File    file       = HighFive::File{filename, HighFive::File::ReadOnly};
    HighFive::Group   group      = file.getGroup("MC");
    HighFive::DataSet hits_table = group.getDataSet("hits");

    // Read a subset of the data back
    hits_table.read(hits);
}
