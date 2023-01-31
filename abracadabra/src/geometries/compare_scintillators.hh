#ifndef geometries_compare_scintillators_hh
#define geometries_compare_scintillators_hh


#include <G4PVPlacement.hh>
#include <G4Types.hh>

G4PVPlacement* compare_scintillators(
    G4bool   use_lxe,
    G4double length,
    G4double scintillator_inner_radius,
    G4double dr_scintillator,
    G4double dr_steel_0,
    G4double dr_vacuum,
    G4double dr_steel_1
);

#endif // geometries_compare_scintillators_hh
