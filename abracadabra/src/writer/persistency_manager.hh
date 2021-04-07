#ifndef persintency_manager_hh
#define persistency_manager_hh 1

#include <G4VPersistencyManager.hh>
#include <G4RunManager.hh>
#include <G4TrajectoryContainer.hh>

#include "hdf5_writer.hh"


class persistency_manager: public G4VPersistencyManager{
	public:
		persistency_manager();
		virtual ~persistency_manager() override {}

		virtual G4bool Store(const G4Event*) override;
		virtual G4bool Store(const G4Run*) override ;
		virtual G4bool Store(const G4VPhysicalVolume*) override;

		virtual G4bool Retrieve(G4Event*&) override;
		virtual G4bool Retrieve(G4Run*&) override;
		virtual G4bool Retrieve(G4VPhysicalVolume*&) override;

		void open_file(G4String);
		void close_file();

	private:
		hdf5_writer* h5writer;
		int nevt;

};

inline G4bool persistency_manager::Retrieve(G4Event*&)           { return false; }
inline G4bool persistency_manager::Retrieve(G4Run*&)             { return false; }
inline G4bool persistency_manager::Retrieve(G4VPhysicalVolume*&) { return false; }

#endif
