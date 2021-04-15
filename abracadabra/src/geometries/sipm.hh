#ifndef geometries_sipm_hh
#define geometries_sipm_hh

#include "nain4.hh"

#include <G4Material.hh>
#include <G4OpticalSurface.hh>
#include <G4ThreeVector.hh>
#include <G4PVPlacement.hh>
#include <string>

// Abstract interface for sepecification and construction of SiPMs
class sipm {
  using vec = std::vector<G4double>;
public:
  sipm(std::string name)
    : name_{name}
    , act{this}
  {}
  G4PVPlacement* build();

  // ----- Builder named parameters (top-level)
#define NEXT return *this;
  sipm& material(G4Material * m)                 { mat = m                        ; NEXT }
  sipm& material(std::string const& name)        { mat = nain4::material(name)    ; NEXT }
  sipm& size(G4double x, G4double y, G4double z) { half = G4ThreeVector{x,y,z} / 2; NEXT }


private:
  struct Active {
    Active(sipm * const body) : body{body} {}

    sipm * const body;
    std::string name_;
    G4Material * mat;
    G4ThreeVector half;
    G4OpticalSurface* active_surface;
    G4MaterialPropertiesTable* active_props;

    Active& name(std::string const& n) { name_ = n; NEXT }
    Active& material(G4Material * m)   { mat   = m; NEXT }
    Active& size(G4double x, G4double y, G4double z) { half = G4ThreeVector{x,y,z} / 2; NEXT }
    Active& skin(std::string const& n);
    sipm& end_active_window() { return *body; }

    template<class... ArgTypes>
    Active& skin(ArgTypes&&... args) {
      active_surface = new G4OpticalSurface{std::forward<ArgTypes>(args)...};
      active_props   = new G4MaterialPropertiesTable{};
      NEXT
    }

    Active& add_property(G4String const& key, vec const& energies, vec const& values) {
      active_props -> AddProperty(key, energies, values);
      NEXT;
    }

    Active& add_property(G4String const& key, vec const& energies, G4double value) {
      active_props -> AddProperty(key, energies, vec(energies.size(), value));
      NEXT;
    }

  };

  struct shifter {};

public:
  Active& active_window() { return act; }

private:
  std::string name_;
  G4Material * mat;
  G4ThreeVector half;
  Active act;
  shifter shift;
};
#undef NEXT

// ===== Sensitive Detector =======================================================
class sipm_sensitive : public G4VSensitiveDetector {
public:
  sipm_sensitive(G4String name) : G4VSensitiveDetector{name} {}
  G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;

public:
  std::vector<G4Step> hits;
};


G4PVPlacement* sipm_hamamatsu_blue(G4bool visible=true);


#endif
