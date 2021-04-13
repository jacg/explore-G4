#ifndef geometries_sipm_hamamatsu_blue_hh
#define geometries_sipm_hamamatsu_blue_hh

#include "g4-mandatory/sensor_hit.hh"

#include <G4VSensitiveDetector.hh>
#include <G4PVPlacement.hh>

//
G4PVPlacement* sipm_hamamatsu_blue(G4bool visible=true);

// ===== Sensitive Detector =======================================================
class hamamatsu_sensitive : public G4VSensitiveDetector {
public:
  hamamatsu_sensitive(G4String name) : G4VSensitiveDetector{name} {}
  void   Initialize (G4HCofThisEvent*)                  override;
  G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;

public:
  sensor_hits_collection* hits;
};


#endif
