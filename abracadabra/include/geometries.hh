#ifndef geometries_hh
#define geometries_hh

#include <G4VPhysicalVolume.hh>

#include <vector>

G4VPhysicalVolume* imas_demonstrator();
G4VPhysicalVolume* nema_phantom(std::vector<G4double> diameters = {10, 13, 17, 22, 28, 37}); // in mm

#endif
