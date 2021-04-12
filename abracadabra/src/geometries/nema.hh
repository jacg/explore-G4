#ifndef geometries_nema_hh
#define geometries_nema_hh

#include "random/random.hh"

#include <G4Event.hh>
#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>
#include <vector>


class nema_phantom {

public:
  G4PVPlacement* geometry() const;
  void generate_primaries(G4Event* event) const;
  G4ThreeVector generate_vertex() const;

  G4ThreeVector sphere_position(unsigned n) const;
  bool inside_a_sphere(G4ThreeVector&) const;
  bool inside_sphere(unsigned, G4ThreeVector&) const;
  bool inside_whole(G4ThreeVector&) const { return true; }
  std::optional<unsigned> in_which_region(G4ThreeVector&) const;

private:
  struct one_sphere {
    one_sphere(G4double diameter, G4double activity) : diameter{diameter}, activity{activity} {}
    G4double diameter;
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
  build_nema_phantom& sphere(G4double diameter, G4double activity);
  nema_phantom build();
};
// ------------------------------------------------------------------------------------

void generate_back_to_back_511_keV_gammas(G4Event* event, G4ThreeVector position, G4double time);

#endif
