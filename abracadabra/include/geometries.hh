#ifndef geometries_hh
#define geometries_hh

#include <G4PVPlacement.hh>

#include <vector>

G4PVPlacement* imas_demonstrator();
G4PVPlacement* nema_phantom(std::vector<G4double> diameters = {10, 13, 17, 22, 28, 37}); // in mm

#endif
