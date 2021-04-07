#include "persistency_manager.hh"


persistency_manager::persistency_manager(){
	std::cout << "Create persistency manager" << std::endl;
	G4String hdf5file = "test_file.h5";
	nevt = 0;
	open_file(hdf5file);
}


void persistency_manager::open_file(G4String filename){
  h5writer = new hdf5_writer();
  h5writer->open(filename);
  return;
}


void persistency_manager::close_file(){
  std::cout << "close file" << std::endl;
  h5writer->close();
  return;
}


G4bool persistency_manager::Store(const G4Event* event){
	std::cout << "Store event" << std::endl;

	auto * trajectories = event->GetTrajectoryContainer();
	for(int i=0; i<trajectories->size(); i++){
		std::cout << "trajectory " << i << std::endl;
	}

	nevt++;
	return true;
}


G4bool persistency_manager::Store(const G4Run* run){
	std::cout << "Store run" << std::endl;

	// Store the number of events to be processed
	auto* app = G4RunManager::GetRunManager();
	G4int num_events = app->GetNumberOfEventsToBeProcessed();
	G4String key = "num_events";
	h5writer->write_run_info(key, std::to_string(num_events).c_str());


	return true;
}

G4bool persistency_manager::Store(const G4VPhysicalVolume* run){
	std::cout << "Store volume" << std::endl;
	return true;
}
