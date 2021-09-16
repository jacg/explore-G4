#ifndef geometries_imas_hh
#define geometries_imas_hh

#include "nain4.hh"

#include <G4PVPlacement.hh>

#include <optional>

G4PVPlacement* imas_demonstrator(n4::sensitive_detector*, G4double length,
                                 G4double quartz_thickness, G4double xenon_thickness,
                                 bool vacuum_before_xenon = false);

void line_cylinder_with_tiles(G4LogicalVolume* cylinder, G4LogicalVolume* sipm,
                              G4double gap, std::optional<G4double> r = {});

#endif
