#ifndef geometries_compare_scintillators_hh
#define geometries_compare_scintillators_hh


#include <G4PVPlacement.hh>
#include <G4String.hh>
#include <G4Types.hh>

G4PVPlacement* compare_scintillators(
    G4String scintillator,
    G4double length,
    G4double scintillator_inner_radius,
    G4double dr_scintillator
);

#endif // geometries_compare_scintillators_hh
