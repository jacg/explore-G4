#ifndef geometries_nema_hh
#define geometries_nema_hh

#include "random/random.hh"

#include <G4Event.hh>
#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>
#include <vector>



// ===== Section 3: Spatial Resolution =======================================================

// This is the most boring phantom of the lot: the geometry consists of 6
// pointlike sources, and there are no materials around them that are supposed
// to attenuate or scatter, so the generator is the only thing that matters.

class nema_3_phantom {
public:
  nema_3_phantom(G4double fov_length);
  void generate_primaries(G4Event* event) const;
  G4PVPlacement* geometry() const;
private:
  const std::vector<G4ThreeVector> vertices;
};

// ===== NEMA NU-2 2018 Section 7: Image Qualitiy, Accuracy of Corrections ==================

// TODO central cylinder is missing, and its attenuating properties are an
// inherent part of the test.

// TODO if the NEMA NU 2 section 7 analyses are to be performed, the body size
// needs to be increased, in order to accommodate the 12 different background
// ROIs. Maybe the shape should then be adjusted to match the official one, but
// maybe it's a pointless PITA.

class nema_7_phantom {

public:
  G4PVPlacement* geometry() const;
  void generate_primaries(G4Event* event) const;
  G4ThreeVector generate_vertex() const;

  G4ThreeVector sphere_position(size_t n) const;
  bool inside_a_sphere(G4ThreeVector&) const;
  bool inside_this_sphere(size_t, G4ThreeVector&) const;
  bool inside_whole(G4ThreeVector&) const { return true; }
  std::optional<size_t> in_which_region(G4ThreeVector&) const;

private:
  struct one_sphere {
    one_sphere(G4double radius, G4double activity) : radius{radius}, activity{activity} {}
    G4double radius;
    G4double activity;
  };

protected:
  std::vector<one_sphere> spheres;
  G4double background  = 1;
  G4double inner_r     = 114.4*mm;
  G4double outer_r     = 152.0*mm;
  G4double half_length =  70.0*mm;
  std::unique_ptr<biased_choice> pick_region;
};

// ----- Builder ----------------------------------------------------------------------
class build_nema_7_phantom : private nema_7_phantom {
public:
  build_nema_7_phantom& length(G4double);
  build_nema_7_phantom& inner_radius(G4double);
  build_nema_7_phantom& outer_radius(G4double);
  build_nema_7_phantom& activity(G4double);
  build_nema_7_phantom& sphere (G4double radius, G4double activity);
  build_nema_7_phantom& sphereD(G4double d     , G4double activity) { return sphere(d/2, activity); }
  nema_7_phantom build();
};
// ------------------------------------------------------------------------------------

// TODO this needs to live elsewhere
void generate_back_to_back_511_keV_gammas(G4Event* event, G4ThreeVector position, G4double time);

#endif
