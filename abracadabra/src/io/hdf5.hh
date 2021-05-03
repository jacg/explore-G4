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

typedef struct {
  double event_id;
  double true_energy;
  double true_r1;
  double true_phi1;
  double true_z1;
  double true_t1;
  double true_r2;
  double true_phi2;
  double true_z2;
  double true_t2;
  double phot_like1;
  double phot_like2;
  double reco_r1;
  double reco_phi1;
  double reco_z1;
  double reco_t1;
  double reco_r2;
  double reco_phi2;
  double reco_z2;
  double reco_t2;
  double not_sel;
} lor_t;


// TODO There's something fishy about the implementation behind this interface:
// it holds on to a filename, and then opens the file each time it wants to
// read.
class hdf5_io {
public:
  hdf5_io(std::string fname);
  ~hdf5_io() { if (open_for_writing) { open_for_writing -> flush(); }}

  void write_run_info(const char* param_key, const char* param_value);
  void write_hit_info(unsigned int evt_id, double x, double y, double z, double t);
  void write_lor_info(double event_id, double energy,
          double r1, double phi1, double z1, double t1,
          double r2, double phi2, double z2, double t2);
  void flush() { if (open_for_writing) { open_for_writing -> flush(); } }

  std::vector<hit_t> read_hit_info();

  static const unsigned CONFLEN = 300;

private:
  void ensure_open_for_writing();

  std::string filename;
  unsigned int runinfo_index;
  unsigned int hit_index;
  unsigned int lor_index;
  std::optional<HighFive::File> open_for_writing;
};


typedef struct {
  char param_key  [hdf5_io::CONFLEN];
  char param_value[hdf5_io::CONFLEN];
} run_info_t;

#endif
