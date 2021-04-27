#include "geometries/nema.hh"

#include <G4VPhysicalVolume.hh>

nema_phantom       a_nema_phantom();
G4VPhysicalVolume* square_array_of_sipms();
G4VPhysicalVolume* cylinder_lined_with_hamamatsus(double length, double radius);
G4VPhysicalVolume* phantom_in_cylinder(nema_phantom const&, double);
