#ifndef IO_HDF5_IO_H
#define IO_HDF5_IO_H

#include <highfive/H5File.hpp>
#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>
#include <highfive/H5DataType.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

// TODO: make this reliable across different architectures
using f32 = float;
using u32 = uint32_t;

// TODO: most of our data don't need 32 bit precision, so we could save a lot of
// space in the tables we write, if only the C++/HDF5 interface could express
// 16-bit numbers.
using f16 = f32;
using u16 = u32;


static const unsigned CONFLEN = 300;


struct hit_t {
  u32 event_id;
  f16 x;
  f16 y;
  f16 z;
  f16 t;
};
HighFive::CompoundType create_hit_type();

struct run_info_t {
  char param_key  [CONFLEN];
  char param_value[CONFLEN];
};
HighFive::CompoundType create_runinfo_type();

struct waveform_t {
  u32 event_id, sensor_id;
  f16 time;
};
HighFive::CompoundType create_waveform_type();

struct total_charge_t {
  u32 event_id, sensor_id;
  u32 charge; // u16 ?
};
HighFive::CompoundType create_total_charge_type();

struct sensor_xyz_t {
  u32 sensor_id;
  f16 x, y, z;
};
HighFive::CompoundType create_sensor_xyz_type();

struct primary_vertex_t {
  u32 event_id;
  f16  x,  y,  z;
  f16 px, py, pz;
};
HighFive::CompoundType create_primary_vertex_type();

struct vertex_t {
  u32 event_id;
  u32 track_id, parent_id;
  f16 x,y,z,t;
  f16 moved;
  f16 pre_KE, post_KE, deposited;
  u32 process_id, volume_id;
};




// TODO There's something fishy about the implementation behind this interface:
// it holds on to a filename, and then opens the file each time it wants to
// read.
class hdf5_io {
public:
  hdf5_io(std::string fname);
  ~hdf5_io() { if (open_for_writing) { open_for_writing -> flush(); }}

  template<class T>
  void write(std::string const& dataset, T const& data);

  void write_run_info(const char* param_key, const char* param_value);
  void write_hit_info    (u32 evt_id, f16 x, f16 y, f16 z, f16 t);
  void write_primary     (u32 evt_id, f16 x, f16 y, f16 z, f16 vx, f16 vy, f16 vz);
  void write_waveform    (u32 evt_id, u32 sensor_id, const std::vector<f16>& times);
  void write_total_charge(u32 evt_id, u32 sensor_id, u32 charge);
  void write_sensor_xyz              (u32 sensor_id, f16 x, f16 y, f16 z);
  void write_vertex(u32 evt_id, u32 track_id, u32 parent_id,
                    f16 x, f16 y, f16 z, f16 t,
                    f16 moved,
                    f16 pre_KE, f16 post_KE, f16 deposited,
                    u32 process_id, u32 volume_id);

  void write_strings(const std::string& dataset_name, const std::vector<std::string>& data);

  void flush() {
    if (open_for_writing) { open_for_writing->flush(); }
  }

  std::vector<hit_t> read_hit_info();

private:
  void ensure_open_for_writing();

  std::string filename;
  std::optional<HighFive::File> open_for_writing;
};

template<class T>
void hdf5_io::write(std::string const& dataset_name, T const& data) {
  unsigned int n_elements = data.size();

  ensure_open_for_writing();
  HighFive::Group   group   = open_for_writing -> getGroup("MC");
  HighFive::DataSet dataset = group.getDataSet(dataset_name);

  // Create extra space in the table and append the new data
  auto index = dataset.getDimensions()[0];
  dataset.resize({index + n_elements});
  dataset.select({index}, {n_elements}).write(data);
}

#endif
