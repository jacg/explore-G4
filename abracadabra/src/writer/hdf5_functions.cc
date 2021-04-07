#include "hdf5_functions.hh"

hsize_t create_run_type(){
  hid_t strtype = H5Tcopy(H5T_C_S1);
  H5Tset_size (strtype, CONFLEN);

  //Create compound datatype for the table
  hsize_t memtype = H5Tcreate (H5T_COMPOUND, sizeof (run_info_t));
  H5Tinsert (memtype, "param_key" , HOFFSET (run_info_t, param_key), strtype);
  H5Tinsert (memtype, "param_value" , HOFFSET (run_info_t, param_value), strtype);
  return memtype;
}

hid_t create_table(hid_t group, std::string& table_name, hsize_t memtype){
  //Create 1D dataspace (evt number). First dimension is unlimited (initially 0)
  const hsize_t ndims = 1;
  hsize_t dims[ndims] = {0};
  hsize_t max_dims[ndims] = {H5S_UNLIMITED};
  hsize_t file_space = H5Screate_simple(ndims, dims, max_dims);

  // Create a dataset creation property list
  // The layout of the dataset have to be chunked when using unlimited dimensions
  hid_t plist = H5Pcreate(H5P_DATASET_CREATE);
  H5Pset_layout(plist, H5D_CHUNKED);
  hsize_t chunk_dims[ndims] = {32768};
  H5Pset_chunk(plist, ndims, chunk_dims);

  //Set compression
  //  int clevel = 0;
  //  H5Pset_deflate (plist, clevel);

  // Create dataset
  hid_t dataset = H5Dcreate(group, table_name.c_str(), memtype, file_space,
                            H5P_DEFAULT, plist, H5P_DEFAULT);

  return dataset;
}

hid_t create_group(hid_t file, std::string& group_name){
  //Create group
  hid_t wfgroup;
  wfgroup = H5Gcreate2(file, group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT,
                       H5P_DEFAULT);
  return wfgroup;
}


void write_run(run_info_t* run_data, hid_t dataset, hid_t memtype, hsize_t counter){
  hid_t memspace, file_space;
  hsize_t dims[1] = {1};
  memspace = H5Screate_simple(1, dims, NULL);

  //Extend dataset
  dims[0] = counter+1;
  H5Dset_extent(dataset, dims);

  file_space = H5Dget_space(dataset);
  hsize_t start[1] = {counter};
  hsize_t count[1] = {1};
  H5Sselect_hyperslab(file_space, H5S_SELECT_SET, start, NULL, count, NULL);
  H5Dwrite(dataset, memtype, memspace, file_space, H5P_DEFAULT, run_data);
  H5Sclose(file_space);
  H5Sclose(memspace);
}
