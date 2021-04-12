#include "hdf5_functions.hh"

#include <vector>

hsize_t create_run_type() {
  hid_t strtype = H5Tcopy(H5T_C_S1);
  H5Tset_size (strtype, CONFLEN);

  //Create compound datatype for the table
  hsize_t memtype = H5Tcreate (H5T_COMPOUND, sizeof (run_info_t));
  H5Tinsert (memtype, "param_key"   , HOFFSET(run_info_t, param_key)  , strtype);
  H5Tinsert (memtype, "param_value" , HOFFSET(run_info_t, param_value), strtype);
  return memtype;
}

hsize_t create_particle_type() {
  hid_t strtype = H5Tcopy(H5T_C_S1);
  H5Tset_size (strtype, STRLEN);

  //Create compound datatype for the table
  hsize_t memtype = H5Tcreate (H5T_COMPOUND, sizeof (particle_t));
  H5Tinsert (memtype, "event_id"          , HOFFSET(particle_t, event_id)          , H5T_NATIVE_INT32);
  H5Tinsert (memtype, "particle_id"       , HOFFSET(particle_t, particle_id)       , H5T_NATIVE_INT);
  H5Tinsert (memtype, "particle_name"     , HOFFSET(particle_t, particle_name)     , strtype);
  H5Tinsert (memtype, "primary"           , HOFFSET(particle_t, primary)           , H5T_NATIVE_CHAR);
  H5Tinsert (memtype, "mother_id"         , HOFFSET(particle_t, mother_id)         , H5T_NATIVE_INT);
  H5Tinsert (memtype, "initial_x"         , HOFFSET(particle_t, initial_x)         , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "initial_y"         , HOFFSET(particle_t, initial_y)         , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "initial_z"         , HOFFSET(particle_t, initial_z)         , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "initial_t"         , HOFFSET(particle_t, initial_t)         , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "final_x"           , HOFFSET(particle_t, final_x)           , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "final_y"           , HOFFSET(particle_t, final_y)           , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "final_z"           , HOFFSET(particle_t, final_z)           , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "final_t"           , HOFFSET(particle_t, final_t)           , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "initial_volume"    , HOFFSET(particle_t, initial_volume)    , strtype);
  H5Tinsert (memtype, "final_volume"      , HOFFSET(particle_t, final_volume)      , strtype);
  H5Tinsert (memtype, "initial_momentum_x", HOFFSET(particle_t, initial_momentum_x), H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "initial_momentum_y", HOFFSET(particle_t, initial_momentum_y), H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "initial_momentum_z", HOFFSET(particle_t, initial_momentum_z), H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "final_momentum_x"  , HOFFSET(particle_t, final_momentum_x)  , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "final_momentum_y"  , HOFFSET(particle_t, final_momentum_y)  , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "final_momentum_z"  , HOFFSET(particle_t, final_momentum_z)  , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "kin_energy"        , HOFFSET(particle_t, kin_energy)        , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "length"            , HOFFSET(particle_t, length)            , H5T_NATIVE_FLOAT);
  H5Tinsert (memtype, "creator_proc"      , HOFFSET(particle_t, creator_proc)      , strtype);
  H5Tinsert (memtype, "final_proc"        , HOFFSET(particle_t, final_proc)        , strtype);
  return memtype;
}

hid_t create_table(hid_t group, std::string const table_name, hsize_t memtype) {
  //Create 1D dataspace (evt number). First dimension is unlimited (initially 0)
  std::vector<hsize_t> dims     = {0};
  std::vector<hsize_t> max_dims = {H5S_UNLIMITED};
  hsize_t file_space = H5Screate_simple(dims.size(), dims.data(), max_dims.data());

  // Create a dataset creation property list
  // The layout of the dataset have to be chunked when using unlimited dimensions
  hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_layout(plist, H5D_CHUNKED);
  std::vector<hsize_t> chunk_dims = {32768};
  H5Pset_chunk(plist, chunk_dims.size(), chunk_dims.data());

  //Set compression
  //  int clevel = 0;
  //  H5Pset_deflate (plist, clevel);

  // Create dataset
  hid_t dataset = H5Dcreate(group, table_name.c_str(), memtype, file_space,
                            H5P_DEFAULT, plist, H5P_DEFAULT);

  return dataset;
}

hid_t create_group(hid_t file, std::string const group_name) {
  hid_t wfgroup;
  wfgroup = H5Gcreate2(file, group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT,
                       H5P_DEFAULT);
  return wfgroup;
}


void write_table_data(void* data, hid_t dataset, hid_t memtype, hsize_t counter)
{
  hid_t memspace, file_space;

  std::vector<hsize_t> dims = {1};
  memspace = H5Screate_simple(dims.size(), dims.data(), nullptr);

  dims[0] = counter+1;
  H5Dset_extent(dataset, dims.data());

  file_space = H5Dget_space(dataset);
  hsize_t start[1] = {counter};
  hsize_t count[1] = {1};
  H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, nullptr, count, nullptr);
  H5Dwrite(dataset, memtype, memspace, file_space, H5P_DEFAULT, data);
  H5Sclose(file_space);
  H5Sclose(memspace);
}
