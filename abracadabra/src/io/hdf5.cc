#include "hdf5.hh"
#include <highfive/H5DataSpace.hpp>

#include <cstring>
#include <iostream>

namespace HF { using namespace HighFive; }

hdf5_io::hdf5_io(std::string fname)
: filename{fname}
, runinfo_index{0}
, hit_index{0}
, waveform_index{0}
, total_charge_index{0}
, q_t0_index{0}
{}

template<class T> using hdf_t = HF::AtomicType<T>;

HF::CompoundType create_hit_type() {
  return {{"event_id", hdf_t<u32>{}},
          {"x"       , hdf_t<f32>{}},
          {"y"       , hdf_t<f32>{}},
          {"z"       , hdf_t<f32>{}},
          {"t"       , hdf_t<f32>{}}};
}
HIGHFIVE_REGISTER_TYPE(hit_t, create_hit_type)

HF::CompoundType create_waveform_type() {
  return {{"event_id" , hdf_t<u32>{}},
          {"sensor_id", hdf_t<u32>{}},
          {"time"     , hdf_t<f32>{}}};
}
HIGHFIVE_REGISTER_TYPE(waveform_t, create_waveform_type)

HF::CompoundType create_total_charge_type() {
  return {{"event_id" , hdf_t<u32>{}},
          {"sensor_id", hdf_t<u32>{}},
          {"charge"   , hdf_t<u32>{}}};
}
HIGHFIVE_REGISTER_TYPE(total_charge_t, create_total_charge_type)

HF::CompoundType create_runinfo_type() {
  return {{"param_key"  , hdf_t<char[hdf5_io::CONFLEN]>{}},
          {"param_value", hdf_t<char[hdf5_io::CONFLEN]>{}}};
}
HIGHFIVE_REGISTER_TYPE(run_info_t, create_runinfo_type)

HF::CompoundType create_sensor_xyz_type() {
  return {{"sensor_id", hdf_t<u32>{}},
          {"x"        , hdf_t<f32>{}},
          {"y"        , hdf_t<f32>{}},
          {"z"        , hdf_t<f32>{}}};
}
HIGHFIVE_REGISTER_TYPE(sensor_xyz_t, create_sensor_xyz_type)

HF::CompoundType create_q_t0_type() {
  return {{"event_id" , hdf_t<u32>{}},
          {"sensor_id", hdf_t<u32>{}},
          {"q"        , hdf_t<u32>{}},
          {"t0"       , hdf_t<f32>{}}};
}
HIGHFIVE_REGISTER_TYPE(q_t0_t, create_q_t0_type)

void set_string_param(char * to, const char * from, u32 max_len) {
  memset(to, 0, max_len);
  strcpy(to, from);
}

run_info_t make_run_info_t(const char* param_key, const char* param_value) {
  run_info_t runinfo;
  set_string_param(runinfo.param_key  , param_key  , hdf5_io::CONFLEN);
  set_string_param(runinfo.param_value, param_value, hdf5_io::CONFLEN);
  return runinfo;
}

void hdf5_io::ensure_open_for_writing() {
  if (open_for_writing) { return; }
  // TODO                                                                              Why truncate?
  open_for_writing = HF::File{filename, HF::File::ReadWrite | HF::File::Create | HF::File::Truncate};
  HF::Group group = open_for_writing->createGroup("MC");

  // To create a table than can be resized it has be of UNLIMITED dimension
  // and requires chunking of the data
  HF::DataSpace dataspace = HF::DataSpace({0}, {HF::DataSpace::UNLIMITED});
  HF::DataSetCreateProps props;
  props.add(HF::Chunking(std::vector<hsize_t>{32768}));

  group.createDataSet("hits"         , dataspace, create_hit_type()         , props);
  group.createDataSet("configuration", dataspace, create_runinfo_type()     , props);
  group.createDataSet("waveform"     , dataspace, create_waveform_type()    , props);
  group.createDataSet("total_charge" , dataspace, create_total_charge_type(), props);
  group.createDataSet("sensor_xyz"   , dataspace, create_sensor_xyz_type()  , props);
  group.createDataSet("q_t0"         , dataspace, create_q_t0_type()        , props);
}


void hdf5_io::write_run_info(const char* param_key, const char* param_value) {
  std::vector<run_info_t> data {make_run_info_t(param_key, param_value)};
  write("configuration", runinfo_index, data);
}

void hdf5_io::write_hit_info(u32 event_id, f32 x, f32 y, f32 z, f32 time) {
  std::vector<hit_t> data{{event_id, x, y, z, time}};
  write("hits", hit_index, data);
}

void hdf5_io::write_waveform(u32 event_id, u32 sensor_id, std::vector<f32> times) {
  std::vector<waveform_t> data;
  for (auto time: times) { data.push_back({event_id, sensor_id, time}); }
  write("waveform", waveform_index, data);
}

void hdf5_io::write_total_charge(u32 event_id, u32 sensor_id, u32 charge) {
  std::vector<total_charge_t> data{{event_id, sensor_id, charge}};
  write("total_charge", total_charge_index, data);
}

void hdf5_io::write_sensor_xyz(u32 sensor_id, f32 x, f32 y, f32 z) {
  // TODO what are the performance implications of doing this one-by-one rather
  // than as a whole vector in one go?
  std::vector<sensor_xyz_t> data{{sensor_id, x, y, z}};
  write("sensor_xyz", hit_index, data);
}

void hdf5_io::write_q_t0(u32 event_id, u32 sensor_id, u32 q, f32 t0) {
  std::vector<q_t0_t> data{{event_id, sensor_id, q, t0}};
  write("q_t0", q_t0_index, data);
}

std::vector<hit_t> hdf5_io::read_hit_info() {
  std::vector<hit_t> hits;
  // Get the table from the file
  HF::File file       = HF::File{filename, HF::File::ReadOnly};
  HF::Group   group      = file.getGroup("MC");
  HF::DataSet hits_table = group.getDataSet("hits");

  hits_table.read(hits);
  return hits;
}
