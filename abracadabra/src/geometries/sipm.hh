#ifndef geometries_sipm_hh
#define geometries_sipm_hh

#include "io/hdf5.hh"
#include "nain4.hh"

#include <G4Material.hh>
#include <G4OpticalSurface.hh>
#include <G4ThreeVector.hh>
#include <G4PVPlacement.hh>
#include <string>


// Abstract interface for sepecification and construction of SiPMs

// Helper for definition of chained methods
#define NEXT return *this;

class sipm;

// ----- active window --------------------------------------------------------------------------
class sipm_active_window {
#define CHAIN sipm_active_window&
  friend class sipm;

public:
  sipm_active_window() = default;
  sipm_active_window(std::string const& name): name{name} {}
  CHAIN thickness(G4double t)                { dz = t                      ; NEXT }
  CHAIN margin   (G4double mx, G4double my)  { margin_x = mx; margin_y = my; NEXT }
  CHAIN material (G4Material* mt)            { mat = mt                    ; NEXT }
  CHAIN vis      (G4VisAttributes const& va) { vis_attributes = va         ; NEXT }

  template<class... ArgTypes>
  CHAIN skin(std::string const& surface_name, G4MaterialPropertiesTable* material_properties, ArgTypes&&... args) {
    active_surface = new G4OpticalSurface{surface_name, std::forward<ArgTypes>(args)...};
    active_props   = material_properties;
    NEXT
  }

private:
  std::string name;
  G4double dz = 0, margin_x = 0, margin_y = 0;
  G4Material*                mat;
  G4MaterialPropertiesTable* active_props;
  G4OpticalSurface*          active_surface;
  G4VisAttributes            vis_attributes;

#undef CHAIN
};

// ----- wavelength shifter ----------------------------------------------------------------------
// TODO
class sipm_wls {
#define CHAIN sipm_wls
public:
private:
#undef CHAIN
};

// ----- SiPM main body --------------------------------------------------------------------------
class sipm {
#define CHAIN sipm&
  using vec = std::vector<G4double>;

public:
  sipm(std::string name, G4VSensitiveDetector* sd) : name{name}, sensitive_detector{sd} {}
  G4LogicalVolume* build();

  using dist = G4double;
  CHAIN material(G4Material * mt)            { mat = mt                       ; NEXT }
  CHAIN material(std::string const& matname) { mat = nain4::material(matname) ; NEXT }
  CHAIN size    (dist x, dist y, dist z)     { half = G4ThreeVector{x,y,z} / 2; NEXT }
  CHAIN vis     (G4VisAttributes const& va)  { vis_attributes = va            ; NEXT }
  CHAIN active  (sipm_active_window a)       { act  = a; NEXT }
  CHAIN wls     (sipm_wls w)                 { wls_ = w; NEXT }
  CHAIN fake_active_material(G4Material* pam) { pre_active_material_  = pam; NEXT }
private:
  std::string             name;
  G4Material *            mat;
  G4ThreeVector           half;
  G4VisAttributes         vis_attributes;
  sipm_active_window      act;
  std::optional<sipm_wls> wls_;
  G4VSensitiveDetector*   sensitive_detector;
  G4Material*             pre_active_material_;
#undef CHAIN
};
#undef NEXT

// ----- Sensitive Detector ------------------------------------------------------------------------
class sipm_sensitive : public G4VSensitiveDetector {
public:
  sipm_sensitive(G4String name) : sipm_sensitive{name, {}} {}
  sipm_sensitive(G4String name, std::optional<std::string> h5_name);
  G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;
  void   EndOfEvent (G4HCofThisEvent*)                  override;

public:
  std::vector<G4Step> hits;
  std::optional<hdf5_io> io; // TODO improve RAII
};

// ----- One example of usage of the interface
G4LogicalVolume* sipm_hamamatsu_blue(G4bool visible, G4VSensitiveDetector*);


#endif
