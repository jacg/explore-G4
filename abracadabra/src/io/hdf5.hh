#ifndef HDF5WRITER_H
#define HDF5WRITER_H

#include <iostream>
#include <string>
#include <vector>

#include <highfive/H5File.hpp>
#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>
#include <highfive/H5DataType.hpp>

#define CONFLEN 300

typedef struct{
    unsigned int event_id;
    double x;
    double y;
    double z;
} hit_t;

typedef struct{
    char param_key  [CONFLEN];
    char param_value[CONFLEN];
} run_info_t;


class hdf5_writer {
public:
    hdf5_writer(std::string fname);
    ~hdf5_writer() {}

    void open();

    void write_run_info(const char* param_key, const char* param_value);
    void write_hit_info(unsigned int evt_id, double x, double y, double z);

    void read_hit_info(std::vector<hit_t>& hits);

private:
    std::string filename;
    unsigned int runinfo_index;
    unsigned int hit_index;
};

#endif
