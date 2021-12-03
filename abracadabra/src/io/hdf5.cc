#include "hdf5.hh"
#include <highfive/H5DataSet.hpp>
#include <highfive/H5DataSpace.hpp>

#include <cstring>
#include <highfive/H5File.hpp>
#include <highfive/H5Group.hpp>
#include <iostream>

namespace HF { using namespace HighFive; }

hdf5_io::hdf5_io(std::string file_name)
: file{ensure_open_for_writing(file_name)}
{}

template<class T> using hdf_t = HF::AtomicType<T>;

HF::CompoundType create_primaries_type() {
  return {{"event_id", hdf_t<u32>{}},
          {"x"       , hdf_t<f16>{}},
          {"y"       , hdf_t<f16>{}},
          {"z"       , hdf_t<f16>{}},
          {"vx"      , hdf_t<f16>{}},
          {"vy"      , hdf_t<f16>{}},
          {"vz"      , hdf_t<f16>{}},
  };
}
HIGHFIVE_REGISTER_TYPE(primaries_t, create_primaries_type)

HF::CompoundType create_vertex_type() {
  return {{ "event_id" , hdf_t<u32>{}},
          { "track_id" , hdf_t<u32>{}},
          {"parent_id" , hdf_t<u32>{}},
          {"x"         , hdf_t<f16>{}},
          {"y"         , hdf_t<f16>{}},
          {"z"         , hdf_t<f16>{}},
          {"t"         , hdf_t<f16>{}},
          {"moved"     , hdf_t<f16>{}},
          {"pre_KE"    , hdf_t<f16>{}},
          {"post_KE"   , hdf_t<f16>{}},
          {"deposited" , hdf_t<f16>{}},
          {"process_id", hdf_t<u32>{}},
          { "volume_id", hdf_t<u32>{}},
  };
}
HIGHFIVE_REGISTER_TYPE(vertex_t, create_vertex_type)

HF::CompoundType create_sensor_xyz_type() {
  return {{"sensor_id", hdf_t<u32>{}},
          {"x"        , hdf_t<f16>{}},
          {"y"        , hdf_t<f16>{}},
          {"z"        , hdf_t<f16>{}}};
}
HIGHFIVE_REGISTER_TYPE(sensor_xyz_t, create_sensor_xyz_type)

HF::CompoundType create_waveform_type() {
  return {{"event_id" , hdf_t<u32>{}},
          {"sensor_id", hdf_t<u32>{}},
          {"time"     , hdf_t<f16>{}}};
}
HIGHFIVE_REGISTER_TYPE(waveform_t, create_waveform_type)

HF::CompoundType create_total_charge_type() {
  return {{"event_id" , hdf_t<u32>{}},
          {"sensor_id", hdf_t<u32>{}},
          {"charge"   , hdf_t<u32>{}}};
}
HIGHFIVE_REGISTER_TYPE(total_charge_t, create_total_charge_type)

HF::CompoundType create_hit_type() {
  return {{"event_id", hdf_t<u32>{}},
          {"x"       , hdf_t<f16>{}},
          {"y"       , hdf_t<f16>{}},
          {"z"       , hdf_t<f16>{}},
          {"t"       , hdf_t<f16>{}}};
}
HIGHFIVE_REGISTER_TYPE(hit_t, create_hit_type)

HF::CompoundType create_runinfo_type() {
  return {{"param_key"  , hdf_t<char[CONFLEN]>{}},
          {"param_value", hdf_t<char[CONFLEN]>{}}};
}
HIGHFIVE_REGISTER_TYPE(run_info_t, create_runinfo_type)

void set_string_param(char * to, const char * from, u32 max_len) {
  memset(to, 0, max_len);
  strcpy(to, from);
}

run_info_t make_run_info_t(const char* param_key, const char* param_value) {
  run_info_t runinfo;
  set_string_param(runinfo.param_key  , param_key  , CONFLEN);
  set_string_param(runinfo.param_value, param_value, CONFLEN);
  return runinfo;
}

HighFive::File hdf5_io::ensure_open_for_writing(std::string const& file_name) {
  return HF::File{file_name, HF::File::ReadWrite | HF::File::Create | HF::File::Truncate};
}

void hdf5_io::write_strings(const std::string& dataset_name, const std::vector<std::string>& data) {
  HF::Group group = file.getGroup("MC");
  // create a dataset adapted to the size of `data`
  HF::DataSet dataset = group.createDataSet<std::string>(dataset_name, HF::DataSpace::From(data));
  dataset.write(data);
}


void hdf5_io::write_run_info(const char* param_key, const char* param_value) {
  buf_run_info({make_run_info_t(param_key, param_value)});
}

void hdf5_io::write_hit_info(u32 event_id, f16 x, f16 y, f16 z, f16 time) {
  buf_hits({event_id, x, y, z, time});
}

void hdf5_io::write_waveform(u32 event_id, u32 sensor_id, const std::vector<f16>& times) {
  for (auto time: times) { buf_waveform({event_id, sensor_id, time}); }
}

void hdf5_io::write_total_charge(u32 event_id, u32 sensor_id, u32 charge) {
  buf_charge({event_id, sensor_id, charge});
}

void hdf5_io::write_sensor_xyz(u32 sensor_id, f16 x, f16 y, f16 z) {
  buf_sensors({sensor_id, x, y, z});
}

void hdf5_io::write_primary(u32 event_id, f16 x, f16 y, f16 z, f16 px, f16 py, f16 pz) {
  buf_primary({event_id, x, y, z, px, py, pz});
}

void hdf5_io::write_vertex(u32 event_id, u32 track_id, u32 parent_id,
                           f16 x, f16 y, f16 z, f16 t,
                           f16 moved,
                           f16 pre_KE, f16 post_KE, f16 deposited,
                           u32 process_id, u32 volume_id) {

  buf_vertex(
    {event_id, track_id, parent_id,
     x,y,z,t,
     moved,
     pre_KE, post_KE, deposited,
     process_id, volume_id
    });
}

std::vector<hit_t> hdf5_io::read_hit_info(std::string const& file_name) {
  std::vector<hit_t> hits;
  // Get the table from the file
  HF::File the_file      = HF::File{file_name, HF::File::ReadOnly};
  HF::Group   group      = the_file.getGroup("MC");
  HF::DataSet hits_table = group.getDataSet("hits");

  hits_table.read(hits);
  return hits;
}

HighFive::DataSet create_dataset(HighFive::File             file,
                                 std::string const&   group_name,
                                 std::string const& dataset_name,
                                 HF::CompoundType const& type,
                                 hsize_t chunk_size) {
  HighFive::Group group =
    file.exist      (group_name) ?
    file.getGroup   (group_name) :
    file.createGroup(group_name) ;

  if (group.exist(dataset_name)) { throw "Dataset " + dataset_name + " already exists"; }

  // To create a table than can be resized it has be of UNLIMITED dimension
  // and requires chunking of the data
  HF::DataSpace empty_unlimited_dataspace = HF::DataSpace({0}, {HF::DataSpace::UNLIMITED});
  HF::DataSetCreateProps create_props;
  create_props.add(HF::Chunking(std::vector<hsize_t>{chunk_size}));
  return group.createDataSet(dataset_name, empty_unlimited_dataspace, type, create_props);
}
