#ifndef HDF5WRITER_H
#define HDF5WRITER_H

#include "hdf5_functions.hh"
#include "g4-mandatory/trajectory.hh"

#include <hdf5.h>
#include <iostream>


class hdf5_writer {
public:
    hdf5_writer();
    ~hdf5_writer() {}

    void open(std::string filename);
    void close();

    void write_run_info(const char* param_key, const char* param_value);
    void write_particle_info(int evt_id, trajectory * trj, bool primary, int mother_id, float kin_energy);

private:
    size_t file_;
    size_t group_;

    //Datasets
    size_t run_table_;
    size_t particle_table_;

    size_t memtype_run_;
    size_t memtype_particle_;

    size_t irun_;
    size_t iparticle_;
};

inline void hdf5_writer::close() { H5Fclose(file_); }

#endif
