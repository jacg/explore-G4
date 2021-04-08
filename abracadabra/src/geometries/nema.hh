#ifndef geometries_nema_hh
#define geometries_nema_hh

#include "random/random.hh"

#include <G4Event.hh>
#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>
#include <vector>


class nema_phantom {

protected:
  nema_phantom(G4double outer_diameter, G4double inner_diameter, G4double activity)
    : background{activity}
    , inner_radius{inner_diameter / 2}
    , outer_radius{outer_diameter / 2}
  {}

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
  G4double background;
  G4double inner_radius;
  G4double outer_radius;
  std::unique_ptr<biased_choice> pick_region;
};

class build_nema_phantom : private nema_phantom {
public:
  build_nema_phantom& sphere(G4double diameter, G4double activity);
  build_nema_phantom() : build_nema_phantom{304*mm, 228.8*mm, 1} {}
  build_nema_phantom(G4double outer_diameter, G4double inner_diameter, G4double activity=1)
    : nema_phantom{outer_diameter, inner_diameter, activity} {}
  nema_phantom build();
};


void generate_back_to_back_511_keV_gammas(G4Event* event, G4ThreeVector position, G4double time);

#endif
