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

static const unsigned CONFLEN = 30;

// HighFive requires the you to register user-defined types that you want it to
// be able to write. This involves two steps:
//
// 1. Write a function which describes the type: the names and types of its
//    members. (I tried to automate this process, but in C++'s metaprogramming
//    features are just too weak, so this is much more trouble than it's worth.)
//
// 2. Register this type and its corresponding describing-function (step 1.)
//    with HighFive.
//
// Step 2. requires calling the macro HIGHFIVE_REGISTER_TYPE, which adds
// (*DEFINES*) a template specialization to HighFive::create_datatype.
//
// This macro cannot be used in header files, because function template
// specialization DEFINITIONS cannot go there. However, when trying to write a
// (template-)polymorphic buffered-writer for HighFive, being templates, these
// need to be defined in the header, and they fail unless the concrete type
// being written has been registered with HighFive.
//
// The solution is to declare (rather than define) the template specialization
// that will be added by HIGHFIVE_REGISTER_TYPE (in the implementation), in the
// header. That is the purpose of the line
//
//   namespace HighFive { template<> DataType create_datatype<TYPE_NAME>(); }
//
// in the following macro.
//
// Additionally, the macro takes care of declaring the function from step 1:
//
//   HighFive::CompoundType FUNCTION();
//
// as this function is used in the templated implementations of buffered writers
// (which must be defined in the header).
//
// Consequently, any user-defined type that we want to use with HighFive should
//
// 1. Be pre-registered, using this macro after its definition in the header.
//
// 2. Have its describing-function from step 1. defined in the implementation.
//
// 3. Be registered (along with its step 1 function) using HIGHFIVE_REGISTER_TYPE
//    just after the function's definition.
#define HIGHFIVE_DECLARATIONS(TYPE_NAME, FUNCTION)   \
  HighFive::CompoundType FUNCTION(); \
  namespace HighFive { template<> DataType create_datatype<TYPE_NAME>(); }

// ----- Table types ------------------------------------------------------------
struct primaries_t {
  u32 event_id;
  f16  x,  y,  z;
  f16 px, py, pz;
};
HIGHFIVE_DECLARATIONS(primaries_t, create_primaries_type)

struct vertex_t {
  u32 event_id;
  u32 track_id, parent_id;
  f16 x,y,z,t;
  f16 moved;
  f16 pre_KE, post_KE, deposited;
  u32 process_id, volume_id;
};
HIGHFIVE_DECLARATIONS(vertex_t, create_vertex_type)

struct sensor_xyz_t {
  u32 sensor_id;
  f16 x, y, z;
};
HIGHFIVE_DECLARATIONS(sensor_xyz_t, create_sensor_xyz_type)

struct waveform_t {
  u32 event_id, sensor_id;
  f16 time;
};
HIGHFIVE_DECLARATIONS(waveform_t, create_waveform_type)

struct total_charge_t {
  u32 event_id, sensor_id;
  u32 charge; // u16 ?
};
HIGHFIVE_DECLARATIONS(total_charge_t, create_total_charge_type)

// TODO Are hit and run_info obsolete legacy noise? If so, remove!
struct hit_t {
  u32 event_id;
  f16 x;
  f16 y;
  f16 z;
  f16 t;
};
HIGHFIVE_DECLARATIONS(hit_t, create_hit_type)

struct run_info_t {
  char param_key  [CONFLEN];
  char param_value[CONFLEN];
};
HIGHFIVE_DECLARATIONS(run_info_t, create_runinfo_type)

#undef HIGHFIVE_DECLARATIONS
// --------------------------------------------------------------------------------

HighFive::DataSet create_dataset(HighFive::File             file,
                                 std::string const&   group_name,
                                 std::string const& dataset_name,
                                 HighFive::CompoundType const& type,
                                 hsize_t chunk_size = 32768);

template<class DATA>
struct write_buffered {

  write_buffered(HighFive::File             file,
                 std::string const&   group_name,
                 std::string const& dataset_name,
                 HighFive::CompoundType const& type,
                 hsize_t chunk_size = 32768)
  : file{file}
  , dataset{create_dataset(file, group_name, dataset_name, type, chunk_size)}
  {}

  ~write_buffered() { flush(); }

  void operator()(DATA&& datum) {
    buffer.push_back(std::move(datum));
    if (buffer.size() >= buffer_size) { flush(); }
  }

private:
  HighFive::File      file;
  std::string   group_name;
  std::string dataset_name;
  std::vector<DATA> buffer;
  size_t chunk_size = 32768; // TODO check whether this size makes sense
  size_t buffer_size = 32 * chunk_size;
  HighFive::DataSet dataset;

  void flush() {

    if (buffer.empty()) { return; }
    size_t n_buffered_elements = buffer.size();
    auto old_size = dataset.getDimensions()[0];
    dataset.resize({old_size +  n_buffered_elements});
    dataset.select({old_size}, {n_buffered_elements}).write(buffer);
    buffer.clear(); // C++ standard guarantees capacity to be unchanged
  }

};


// TODO There's something fishy about the implementation behind this interface:
// it holds on to a filename, and then opens the file each time it wants to
// read.
class hdf5_io {
public:
  hdf5_io(std::string file_name);
  ~hdf5_io() { file.flush(); }

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

  // NOTE Only used in one test, so far
  static std::vector<hit_t> read_hit_info(std::string const& file_name);

private:
  HighFive::File ensure_open_for_writing(std::string const& file_name);

  HighFive::File file;

public:
  write_buffered<    run_info_t> buf_run_info{file, "MC", "run_info"    , create_runinfo_type     ()};
  write_buffered<         hit_t> buf_hits    {file, "MC", "hits"        , create_hit_type         ()};
  write_buffered<    waveform_t> buf_waveform{file, "MC", "waveform"    , create_waveform_type    ()};
  write_buffered<total_charge_t> buf_charge  {file, "MC", "total_charge", create_total_charge_type()};
  write_buffered<  sensor_xyz_t> buf_sensors {file, "MC", "sensor_xyz"  , create_sensor_xyz_type  ()};
  write_buffered<   primaries_t> buf_primary {file, "MC", "primaries"   , create_primaries_type   ()};
  write_buffered<      vertex_t> buf_vertex  {file, "MC", "vertices"    , create_vertex_type      ()};
};

template<class T>
void hdf5_io::write(std::string const& dataset_name, T const& data) {
  unsigned int n_elements = data.size();

  HighFive::Group   group   = file.getGroup("MC");
  HighFive::DataSet dataset = group.getDataSet(dataset_name);

  // Create extra space in the table and append the new data
  auto index = dataset.getDimensions()[0];
  dataset.resize({index + n_elements});
  dataset.select({index}, {n_elements}).write(data);
}

#endif
