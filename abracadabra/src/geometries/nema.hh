#ifndef geometries_nema_hh
#define geometries_nema_hh

#include "random/random.hh"

#include <G4Event.hh>
#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>
#include <vector>



// TODO central cylinder is missing, and its attenuating properties are an
// inherent part of the test.

// TODO if the NEMA NU 2 section 7 analyses are to be performed, the body size
// needs to be increased, in order to accommodate the 12 different background
// ROIs. Maybe the shape should then be adjusted to match the official one, but
// maybe it's a pointless PITA.

// TODO needs to be renamed to something like nema_image_quality_phantom

class nema_phantom {

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
class build_nema_phantom : private nema_phantom {
public:
  build_nema_phantom& length(G4double);
  build_nema_phantom& inner_radius(G4double);
  build_nema_phantom& outer_radius(G4double);
  build_nema_phantom& activity(G4double);
  build_nema_phantom& sphere(G4double radius, G4double activity);
  nema_phantom build();
};
// ------------------------------------------------------------------------------------

// TODO this needs to live elsewhere
void generate_back_to_back_511_keV_gammas(G4Event* event, G4ThreeVector position, G4double time);

#endif
