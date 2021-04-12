#include "hdf5_writer.hh"

#include <cstring>


hdf5_writer::hdf5_writer()
: file_{0}
, group_{0}
, run_table_{0}
, particle_table_{0}
, memtype_run_{0}
, memtype_particle_{0}
, irun_{0}
, iparticle_{0} {}


void hdf5_writer::open(std::string file_name) {
    file_ = H5Fcreate(file_name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    group_          = create_group(file_, "/MC");
    run_table_      = create_table(group_, "configuration", memtype_run_      = create_run_type());
    particle_table_ = create_table(group_, "particles",     memtype_particle_ = create_particle_type());

}

void hdf5_writer::write_run_info(const char* param_key, const char* param_value) {
    run_info_t run_data;
    memset(run_data.param_key,   0, CONFLEN);
    memset(run_data.param_value, 0, CONFLEN);
    strcpy(run_data.param_key, param_key);
    strcpy(run_data.param_value, param_value);
    write_table_data((void*) &run_data, run_table_, memtype_run_, irun_);

    irun_++;
}


void hdf5_writer::write_particle_info(int evt_id, trajectory * trj, bool primary, int mother_id, float kin_energy) {
    particle_t particle;

    particle.event_id    = evt_id;
    particle.particle_id = trj->GetTrackID();
    particle.primary     = (char) primary;
    particle.mother_id   = mother_id;
    particle.kin_energy  = kin_energy;
    particle.length      = trj->GetTrackLength();

    // Initialize strings with 0's
    memset(particle.particle_name , 0, STRLEN);
    memset(particle.initial_volume, 0, STRLEN);
    memset(particle.final_volume  , 0, STRLEN);
    memset(particle.creator_proc  , 0, STRLEN);
    memset(particle.final_proc    , 0, STRLEN);

    // Copy actual values
    strcpy(particle.particle_name , trj->GetParticleName()  .c_str());
    strcpy(particle.initial_volume, trj->GetInitialVolume() .c_str());
    strcpy(particle.final_volume  , trj->GetFinalVolume()   .c_str());
    strcpy(particle.creator_proc  , trj->GetCreatorProcess().c_str());
    strcpy(particle.final_proc    , trj->GetFinalProcess()  .c_str());

    particle.initial_x = trj->GetInitialPosition().x();
    particle.initial_y = trj->GetInitialPosition().y();
    particle.initial_z = trj->GetInitialPosition().z();
    particle.initial_t = trj->GetInitialTime();

    particle.final_x = trj->GetFinalPosition().x();
    particle.final_y = trj->GetFinalPosition().y();
    particle.final_z = trj->GetFinalPosition().z();
    particle.final_t = trj->GetFinalTime();

    particle.initial_momentum_x = trj->GetInitialMomentum().x();
    particle.initial_momentum_y = trj->GetInitialMomentum().y();
    particle.initial_momentum_z = trj->GetInitialMomentum().z();

    particle.final_momentum_x = trj->GetFinalMomentum().x();
    particle.final_momentum_y = trj->GetFinalMomentum().y();
    particle.final_momentum_z = trj->GetFinalMomentum().z();

    write_table_data((void*) &particle, particle_table_, memtype_particle_, iparticle_);
    iparticle_++;
}
