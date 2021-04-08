#include "persistency_manager.hh"
#include "g4-mandatory/trajectory.hh"


persistency_manager::persistency_manager() {
    std::cout << "Create persistency manager" << std::endl;
    G4String hdf5file = "test_file.h5";
    nevt = 0;
    open_file(hdf5file);
}


void persistency_manager::open_file(G4String filename) {
    h5writer = new hdf5_writer();
    h5writer->open(filename);
}


void persistency_manager::close_file() {
    std::cout << "close file" << std::endl;
    h5writer->close();
}


G4bool persistency_manager::Store(const G4Event* event) {
    std::cout << "Store event" << std::endl;

    G4TrajectoryContainer* trajectories = event->GetTrajectoryContainer();
    if (trajectories) {
        store_trajectories(trajectories);
    }

    nevt++;
    return true;
}


G4bool persistency_manager::Store(const G4Run* run) {
    std::cout << "Store run" << std::endl;

    // Store the number of events to be processed
    auto* app = G4RunManager::GetRunManager();
    G4int num_events = app->GetNumberOfEventsToBeProcessed();
    G4String key = "num_events";
    h5writer->write_run_info(key, std::to_string(num_events).c_str());


    return true;
}

G4bool persistency_manager::Store(const G4VPhysicalVolume* run) {
    std::cout << "Store volume" << std::endl;
    return true;
}


void persistency_manager::store_trajectories(G4TrajectoryContainer* trajectories) {
    for(int i=0; i<trajectories->size(); i++) {
        std::cout << "trajectory " << i << std::endl;

        trajectory* trj = dynamic_cast<trajectory*>((*trajectories)[i]);
        if (!trj) continue;

        G4double      mass    = trj->GetParticleDefinition()->GetPDGMass();
        G4ThreeVector ini_mom = trj->GetInitialMomentum();
        G4double      energy  = sqrt(ini_mom.mag2() + mass*mass);

        float kin_energy = energy - mass;
        char primary = 0;
        G4int mother_id = 0;
        if (!trj->GetParentID()) {
            primary = 1;
        } else {
            mother_id = trj->GetParentID();
        }

        h5writer->write_particle_info(nevt, trj, primary, mother_id, kin_energy);
    }
}
