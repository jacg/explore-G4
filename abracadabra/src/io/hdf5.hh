#ifndef IO_HDF5_IO_H
#define IO_HDF5_IO_H

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
  double t;
} hit_t;


// TODO There's something fishy about the implementation behind this interface:
// it holds on to a filename, and then opens the file each time it wants to
// read.
class hdf5_io {
public:
  hdf5_io(std::string fname);
  ~hdf5_io() { if (open_for_writing) { open_for_writing -> flush(); }}

  void write_run_info(const char* param_key, const char* param_value);
  void write_hit_info(unsigned int evt_id, double x, double y, double z, double t);
  void write_waveform(unsigned int evt_id, unsigned int sensor_id, std::vector<double> times);
  void write_total_charge(unsigned int evt_id, unsigned int sensor_id, double charge);
  void flush() { if (open_for_writing) { open_for_writing -> flush(); } }

  std::vector<hit_t> read_hit_info();

  static const unsigned CONFLEN = 300;

private:
  void ensure_open_for_writing();

  std::string filename;
  unsigned int runinfo_index;
  unsigned int hit_index;
  unsigned int waveform_index;
  unsigned int total_charge_index;
  std::optional<HighFive::File> open_for_writing;
};


typedef struct {
  char param_key  [hdf5_io::CONFLEN];
  char param_value[hdf5_io::CONFLEN];
} run_info_t;


typedef struct {
  unsigned int event_id;
  unsigned int sensor_id;
  double time;
} waveform_t;

typedef struct {
  unsigned int event_id;
  unsigned int sensor_id;
  double charge;
} total_charge_t;


#endif
