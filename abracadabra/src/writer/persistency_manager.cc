#include "persistency_manager.hh"
#include "g4-mandatory/sensor_hit.hh"

persistency_manager::persistency_manager() {
    nevt = 0;
    open_file("test_file.h5");
}

void persistency_manager::open_file(G4String filename) {
    h5writer = new hdf5_writer(filename);
    h5writer->open();
}


G4bool persistency_manager::Store(const G4Event* event) {
    store_hits(event->GetHCofThisEvent());
    nevt++;
    return true;
}


G4bool persistency_manager::Store(const G4Run* run) {
    // Store the number of events to be processed
    auto* app = G4RunManager::GetRunManager();
    G4int num_events = app->GetNumberOfEventsToBeProcessed();
    G4String key = "num_events";
    h5writer->write_run_info(key.c_str(), std::to_string(num_events).c_str());
    return true;
}

void persistency_manager::store_hits(G4HCofThisEvent* hit_collections) {
    if (!hit_collections) { return; }

    unsigned int n_collections = hit_collections->GetNumberOfCollections();

    for (unsigned int i=0; i<n_collections; i++) {
        auto hits = hit_collections->GetHC(i);
        unsigned int n_hits = hits->GetSize();
        for (unsigned int j=0; j<n_hits; j++) {
            sensor_hit* hit = (sensor_hit*) hits->GetHit(j);
            h5writer->write_hit_info(nevt,
                    hit->get_position().x(),
                    hit->get_position().y(),
                    hit->get_position().z());
        }
    }
}
