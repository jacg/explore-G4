#include "persistency_manager.hh"


persistency_manager::persistency_manager() {
    nevt = 0;
    open_file("test_file.h5");
}

void persistency_manager::open_file(G4String filename) {
    h5writer = new hdf5_writer();
    h5writer->open(filename);
}


void persistency_manager::close_file() {
    h5writer->close();
}


G4bool persistency_manager::Store(const G4Event* event) {
	std::cout << "Store event info" << std::endl;
    nevt++;
    return true;
}


G4bool persistency_manager::Store(const G4Run* run) {
    // Store the number of events to be processed
    auto* app = G4RunManager::GetRunManager();
    G4int num_events = app->GetNumberOfEventsToBeProcessed();
    G4String key = "num_events";
    h5writer->write_run_info(key, std::to_string(num_events).c_str());


    return true;
}
