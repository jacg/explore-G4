#include "geometries/nema.hh"

#include <G4VPhysicalVolume.hh>
#include <G4VSensitiveDetector.hh>

nema_7_phantom     a_nema_phantom();
G4VPhysicalVolume* square_array_of_sipms(G4VSensitiveDetector* = nullptr);
G4VPhysicalVolume* cylinder_lined_with_hamamatsus(double length, double radius, double dr_Xe,
                                                  G4VSensitiveDetector* = nullptr);
