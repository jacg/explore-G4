#ifndef geometries_nema_hh
#define geometries_nema_hh

#include <G4Event.hh>
#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>
#include <vector>


class nema_phantom {

private:
  struct one_sphere {
    one_sphere(G4double radius, G4double activity) : radius{radius}, activity{activity} {}
    G4double radius;
    G4double activity;
  };

public:

  G4PVPlacement* geometry() const;

  void generate_primaries(G4Event* event);

  nema_phantom& sphere(G4double radius, G4double activity) {
    spheres.emplace_back(radius, activity);
    return *this;
  };


private:
  std::vector<one_sphere> spheres;
  G4double background;
  G4double inner_radius = 114.4 * mm;
  G4double outer_radius = 152   * mm;
};



#endif
