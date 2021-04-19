#ifndef HDF5WRITER_H
#define HDF5WRITER_H

#include <iostream>
#include <string>
#include <vector>

#include <highfive/H5File.hpp>
#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>
#include <highfive/H5DataType.hpp>


typedef struct {
  unsigned int event_id;
  double x;
  double y;
  double z;
} hit_t;


class hdf5_io {
public:
  hdf5_io(std::string fname);
  ~hdf5_io() {} // TODO improve RAII

  void open();

  void write_run_info(const char* param_key, const char* param_value);
  void write_hit_info(unsigned int evt_id, double x, double y, double z);

  void read_hit_info(std::vector<hit_t>& hits);

  static const unsigned CONFLEN = 300;

private:
  std::string filename;
  unsigned int runinfo_index;
  unsigned int hit_index;
};


typedef struct {
  char param_key  [hdf5_io::CONFLEN];
  char param_value[hdf5_io::CONFLEN];
} run_info_t;


#endif
