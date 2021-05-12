#ifndef IO_HDF5_IO_H
#define IO_HDF5_IO_H

#include <iostream>
#include <string>
#include <vector>

#include <highfive/H5File.hpp>
#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>
#include <highfive/H5DataType.hpp>


struct hit_t {
  unsigned int event_id;
  double x;
  double y;
  double z;
  double t;
};


// TODO There's something fishy about the implementation behind this interface:
// it holds on to a filename, and then opens the file each time it wants to
// read.
class hdf5_io {
public:
  hdf5_io(std::string fname);
  ~hdf5_io() { if (open_for_writing) { open_for_writing -> flush(); }}

  template<class T>
  void write(std::string const& dataset, unsigned int& index, T const& data);

  void write_run_info(const char* param_key, const char* param_value);
  void write_hit_info(unsigned int evt_id, double x, double y, double z, double t);
  void write_waveform(unsigned int evt_id, unsigned int sensor_id, std::vector<double> times);
  void write_total_charge(unsigned int evt_id, unsigned int sensor_id, size_t charge);
  void write_sensor_xyz(unsigned int sensor_id, double x, double y, double z);
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

template<class T>
void hdf5_io::write(std::string const& dataset, unsigned int& index, T const& data) {
  unsigned int n_elements = data.size();

  ensure_open_for_writing();
  HighFive::Group   group      = open_for_writing -> getGroup("MC");
  HighFive::DataSet hits_table = group.getDataSet(dataset);

  // Create extra space in the table and append the new data
  hits_table.resize({index + n_elements});
  hits_table.select({index}, {n_elements}).write(data);

  index += n_elements;
}

struct run_info_t {
  char param_key  [hdf5_io::CONFLEN];
  char param_value[hdf5_io::CONFLEN];
};

struct waveform_t {
  unsigned int event_id;
  unsigned int sensor_id;
  double time;
};

struct total_charge_t {
  unsigned int event_id;
  unsigned int sensor_id;
  size_t charge;
};

struct sensor_xyz_t {
  unsigned int sensor_id;
  double x;
  double y;
  double z;
};

#endif
