#ifndef geometries_imas_hh
#define geometries_imas_hh

#include "nain4.hh"

#include <G4PVPlacement.hh>

G4PVPlacement* imas_demonstrator(n4::sensitive_detector*);

void line_cylinder_with_tiles(G4LogicalVolume* cylinder, G4LogicalVolume* sipm, G4double gap);

#endif
