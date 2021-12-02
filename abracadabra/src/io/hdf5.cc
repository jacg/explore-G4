#include "hdf5.hh"
#include <highfive/H5DataSpace.hpp>

#include <cstring>
#include <iostream>

namespace HF { using namespace HighFive; }

hdf5_io::hdf5_io(std::string fname)
: filename{fname}
{}

template<class T> using hdf_t = HF::AtomicType<T>;

HF::CompoundType create_primary_vertex_type() {
  return {{"event_id", hdf_t<u32>{}},
          {"x"       , hdf_t<f16>{}},
          {"y"       , hdf_t<f16>{}},
          {"z"       , hdf_t<f16>{}},
          {"vx"      , hdf_t<f16>{}},
          {"vy"      , hdf_t<f16>{}},
          {"vz"      , hdf_t<f16>{}},
  };
}
HIGHFIVE_REGISTER_TYPE(primary_vertex_t, create_primary_vertex_type)

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

void hdf5_io::ensure_open_for_writing() {
  if (file) { return; }
  // TODO                                                                              Why truncate?
  file = HF::File{filename, HF::File::ReadWrite | HF::File::Create | HF::File::Truncate};

  HF::Group group = file->createGroup("MC");

  // To create a table than can be resized it has be of UNLIMITED dimension
  // and requires chunking of the data
  HF::DataSpace dataspace = HF::DataSpace({0}, {HF::DataSpace::UNLIMITED});
  HF::DataSetCreateProps props;
  props.add(HF::Chunking(std::vector<hsize_t>{32768}));

  group.createDataSet("hits"         , dataspace, create_hit_type()           , props);
  group.createDataSet("configuration", dataspace, create_runinfo_type()       , props);
  group.createDataSet("waveform"     , dataspace, create_waveform_type()      , props);
  group.createDataSet("total_charge" , dataspace, create_total_charge_type()  , props);
  group.createDataSet("sensor_xyz"   , dataspace, create_sensor_xyz_type()    , props);
  group.createDataSet("primaries"    , dataspace, create_primary_vertex_type(), props);
  group.createDataSet("vertices"     , dataspace, create_vertex_type()        , props);
}

void hdf5_io::write_strings(const std::string& dataset_name, const std::vector<std::string>& data) {
  ensure_open_for_writing();
  HF::Group group = file -> getGroup("MC");
  // create a dataset adapted to the size of `data`
  HF::DataSet dataset = group.createDataSet<std::string>(dataset_name, HF::DataSpace::From(data));
  dataset.write(data);
}


void hdf5_io::write_run_info(const char* param_key, const char* param_value) {
  std::vector<run_info_t> data {make_run_info_t(param_key, param_value)};
  write("configuration", data);
}

void hdf5_io::write_hit_info(u32 event_id, f16 x, f16 y, f16 z, f16 time) {
  std::vector<hit_t> data{{event_id, x, y, z, time}};
  write("hits", data);
}

void hdf5_io::write_waveform(u32 event_id, u32 sensor_id, const std::vector<f16>& times) {
  std::vector<waveform_t> data;
  for (auto time: times) { data.push_back({event_id, sensor_id, time}); }
  write("waveform", data);
}

void hdf5_io::write_total_charge(u32 event_id, u32 sensor_id, u32 charge) {
  std::vector<total_charge_t> data{{event_id, sensor_id, charge}};
  write("total_charge", data);
}

void hdf5_io::write_sensor_xyz(u32 sensor_id, f16 x, f16 y, f16 z) {
  // TODO what are the performance implications of doing this one-by-one rather
  // than as a whole vector in one go?
  std::vector<sensor_xyz_t> data{{sensor_id, x, y, z}};
  write("sensor_xyz", data);
}

void hdf5_io::write_primary(u32 event_id, f16 x, f16 y, f16 z, f16 px, f16 py, f16 pz) {
  std::vector<primary_vertex_t> data{{event_id, x, y, z, px, py, pz}};
  write("primaries", data);
}

void hdf5_io::write_vertex(u32 event_id, u32 track_id, u32 parent_id,
                           f16 x, f16 y, f16 z, f16 t,
                           f16 moved,
                           f16 pre_KE, f16 post_KE, f16 deposited,
                           u32 process_id, u32 volume_id) {
  std::vector<vertex_t> data{
    {event_id, track_id, parent_id,
     x,y,z,t,
     moved,
     pre_KE, post_KE, deposited,
     process_id, volume_id
    }};
  write("vertices", data);
}

std::vector<hit_t> hdf5_io::read_hit_info() {
  std::vector<hit_t> hits;
  // Get the table from the file
  HF::File the_file      = HF::File{filename, HF::File::ReadOnly};
  HF::Group   group      = the_file.getGroup("MC");
  HF::DataSet hits_table = group.getDataSet("hits");

  hits_table.read(hits);
  return hits;
}
