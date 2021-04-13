#ifndef persintency_manager_hh
#define persistency_manager_hh 1

#include "hdf5_writer.hh"

#include <G4VPersistencyManager.hh>
#include <G4RunManager.hh>


class persistency_manager: public G4VPersistencyManager {
public:
    persistency_manager();
    virtual ~persistency_manager() override { this->close_file(); };

    G4bool Store(const G4Event*)           override;
    G4bool Store(const G4Run*)             override;
    G4bool Store(const G4VPhysicalVolume*) override { return true; }

    G4bool Retrieve(G4Event*&)           override { return false; }
    G4bool Retrieve(G4Run*&)             override { return false; }
    G4bool Retrieve(G4VPhysicalVolume*&) override { return false; }

    void open_file(G4String);
    void close_file();

    void store_hits(G4HCofThisEvent*);

private:
    hdf5_writer* h5writer;
    int nevt;
};

#endif
