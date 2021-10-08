#ifndef geometries_jaszczak_hh
#define geometries_jaszczak_hh

#include "geometries/generate_primaries.hh"
#include "geometries/inspect.hh"

#include "nain4.hh"

#include <G4PVPlacement.hh>
#include <G4SystemOfUnits.hh>

#include <memory>

class jaszczak_phantom {
  using D = G4double;
public:
  jaszczak_phantom(std::unique_ptr<G4RunManager>& rm) : run_manager{&rm} {}
  G4PVPlacement* geometry() const;
  void generate_primaries(G4Event* event) const { return ::generate_primaries(*this, event); }
  G4ThreeVector generate_vertex() const;

protected:
  D height_cylinder = 186 * mm;
  D radius_cylinder = 216 * mm / 2;
  std::vector<D> radii_rods    = n4::scale_by(mm/2, {9.5, 12.7, 15.9, 19.1, 25.4, 31.8});
  std::vector<D> radii_spheres = n4::scale_by(mm/2, {3.2,  4.8,  6.4,  7.9,  9.5, 11.1});
  D height_rods    =  88   * mm;
  D height_spheres = 127   * mm;
  D gap            =  14.4 * mm; // Width of corridors between groups of rods
  D margin         =   0.1 * mm;
private:
  void rod_sector(unsigned long n, G4double r, G4LogicalVolume* cylinder, G4Material*) const;


  std::unique_ptr<G4RunManager>* run_manager; // Store as pointer rather than
                                              // reference, to avoid implicit
                                              // deletion of copy constructor,
                                              // which is needed use in
                                              // std::variant
  std::unique_ptr<world_geometry_inspector> inspector_{};
  world_geometry_inspector* inspector();
};

// ----- Builder ----------------------------------------------------------------------
class build_jaszczak_phantom : public jaszczak_phantom {
  using T = build_jaszczak_phantom;
  using D = G4double;
public:
  T& cylinder_height  (D h) { height_cylinder = h; return *this; }
  T& cylinder_radius  (D r) { radius_cylinder = r; return *this; }
  T&     rod_radii    (D a, D b, D c, D d, D e, D f) { radii_rods    = {a,b,c,d,e,f}; return *this; }
  T&  sphere_radii    (D a, D b, D c, D d, D e, D f) { radii_spheres = {a,b,c,d,e,f}; return *this; }
  T&     rod_height   (D h) { height_rods    = h; return *this;}
  T&  sphere_height   (D h) { height_spheres = h; return *this;}
  T& cylinder_diameter(D d)                          { return cylinder_radius(d/2); }
  T&     rod_diameters(D a, D b, D c, D d, D e, D f) { return    rod_radii(a/2, b/2, c/2, d/2, e/2, f/2); }
  T&  sphere_diameters(D a, D b, D c, D d, D e, D f) { return sphere_radii(a/2, b/2, c/2, d/2, e/2, f/2); }
  jaszczak_phantom build();
};
#endif
