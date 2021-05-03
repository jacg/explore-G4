#include "hdf5.hh"
#include <highfive/H5DataSpace.hpp>

#include <cstring>
#include <iostream>

namespace HF { using namespace HighFive; }

hdf5_io::hdf5_io(std::string fname)
: filename{fname}
, runinfo_index{0}
, hit_index{0}
, lor_index{0} {}


HF::CompoundType create_hit_type() {
  return {{"event_id", HF::AtomicType<unsigned int>{}},
          {"x", HF::AtomicType<double>{}},
          {"y", HF::AtomicType<double>{}},
          {"z", HF::AtomicType<double>{}},
          {"t", HF::AtomicType<double>{}}};
}
HIGHFIVE_REGISTER_TYPE(hit_t, create_hit_type)

HF::CompoundType create_runinfo_type() {
  return {{"param_key"  , HF::AtomicType<char[hdf5_io::CONFLEN]>{}},
          {"param_value", HF::AtomicType<char[hdf5_io::CONFLEN]>{}}};
}
HIGHFIVE_REGISTER_TYPE(run_info_t, create_runinfo_type)


HF::CompoundType create_lor_type() {
  return {{"event_id"   , HF::AtomicType<double>{}},
          {"true_energy", HF::AtomicType<double>{}},
          {"true_r1"    , HF::AtomicType<double>{}},
          {"true_phi1"  , HF::AtomicType<double>{}},
          {"true_z1"    , HF::AtomicType<double>{}},
          {"true_t1"    , HF::AtomicType<double>{}},
          {"true_r2"    , HF::AtomicType<double>{}},
          {"true_phi2"  , HF::AtomicType<double>{}},
          {"true_z2"    , HF::AtomicType<double>{}},
          {"true_t2"    , HF::AtomicType<double>{}},
          {"phot_like1" , HF::AtomicType<double>{}},
          {"phot_like2" , HF::AtomicType<double>{}},
          {"reco_r1"    , HF::AtomicType<double>{}},
          {"reco_phi1"  , HF::AtomicType<double>{}},
          {"reco_z1"    , HF::AtomicType<double>{}},
          {"reco_t1"    , HF::AtomicType<double>{}},
          {"reco_r2"    , HF::AtomicType<double>{}},
          {"reco_phi2"  , HF::AtomicType<double>{}},
          {"reco_z2"    , HF::AtomicType<double>{}},
          {"reco_t2"    , HF::AtomicType<double>{}},
          {"not_sel"    , HF::AtomicType<double>{}}};
}
HIGHFIVE_REGISTER_TYPE(lor_t, create_lor_type)


void set_string_param(char * to, const char * from, unsigned int max_len) {
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

  group.createDataSet("hits"         , dataspace, create_hit_type()    , props);
  group.createDataSet("configuration", dataspace, create_runinfo_type(), props);

  group = open_for_writing->createGroup("reco_info");
  group.createDataSet("table", dataspace, create_lor_type()    , props);
}

void hdf5_io::write_run_info(const char* param_key, const char* param_value) {
  std::vector<run_info_t> data {make_run_info_t(param_key, param_value)};
  unsigned int n_elements = data.size();

  ensure_open_for_writing();
  HF::Group   group      = open_for_writing -> getGroup("MC");
  HF::DataSet hits_table = group.getDataSet("configuration");

  // Create extra space in the table and append the new data
  hits_table.resize({runinfo_index + n_elements});
  hits_table.select({runinfo_index}, {n_elements}).write(data);

  runinfo_index += n_elements;
}

void hdf5_io::write_hit_info(unsigned int event_id, double x, double y, double z, double time) {
  std::vector<hit_t> data{{event_id, x, y, z, time}};
  unsigned int n_elements = data.size();

  ensure_open_for_writing();
  HF::Group   group      = open_for_writing -> getGroup("MC");
  HF::DataSet hits_table = group.getDataSet("hits");

  // Create extra space in the table and append the new data
  hits_table.resize({hit_index + n_elements});
  hits_table.select({hit_index}, {n_elements}).write(data);

  hit_index += n_elements;
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


void hdf5_io::write_lor_info(double event_id, double energy,
          double r1, double phi1, double z1, double t1,
          double r2, double phi2, double z2, double t2){
  lor_t lor;
  lor.event_id    = event_id;
  lor.true_energy = energy;
  lor.true_r1     = r1;
  lor.true_phi1   = phi1;
  lor.true_z1     = z1;
  lor.true_t1     = t1;
  lor.true_r2     = r2;
  lor.true_phi2   = phi2;
  lor.true_z2     = z2;
  lor.true_t2     = t2;
  lor.phot_like1  = 1;
  lor.phot_like2  = 1;
  lor.reco_r1     = r1;
  lor.reco_phi1   = phi1;
  lor.reco_z1     = z1;
  lor.reco_t1     = t1;
  lor.reco_r2     = r2;
  lor.reco_phi2   = phi2;
  lor.reco_z2     = z2;
  lor.reco_t2     = t2;
  lor.not_sel     = 0;

  std::vector<lor_t> data{lor};
  unsigned int n_elements = data.size();

  ensure_open_for_writing();
  HF::Group   group = open_for_writing -> getGroup("reco_info");
  HF::DataSet table = group.getDataSet("table");

  // Create extra space in the table and append the new data
  table.resize({lor_index + n_elements});
  table.select({lor_index}, {n_elements}).write(data);

  lor_index += n_elements;
}
