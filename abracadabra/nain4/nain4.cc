#include "nain4.hh"

#include <G4NistManager.hh>
#include <G4PVPlacement.hh>
#include <G4String.hh>

namespace nain4 {

G4Material* material(G4String const& name) {
  return G4NistManager::Instance()->FindOrBuildMaterial(name);
};

G4Element* element(G4String const& name) {
  return G4NistManager::Instance()->FindOrBuildElement(name);
};

G4Material* material_from_elements(std::string name,
                                   G4double density,
                                   G4State state,
                                   std::vector<std::tuple<std::string, int>> components) {
  auto n_components = static_cast<G4int>(components.size());
  auto the_material = new G4Material{name, density, n_components, state};
  for (auto [the_element, n_atoms]: components) {
    the_material -> AddElement(element(the_element), n_atoms);
  }
  return the_material;
}

G4PVPlacement* place::now() {
  // ----- Name --------------------------------------------------
  // + By default, the name is copied from the child volume.
  // + If a copy_number is specified, it is appended to the name.
  // + All of this is overriden if a name is provided explicitly.
  G4String the_name;
  if (this->label) {
    the_name = this->label.value();
  } else {
    the_name = this->child.value()->GetName();
    if (this->copy_number) {
      auto suffix = "-" + std::to_string(copy_number.value());
      the_name += suffix;
    }
  }
  // TODO: Think about these later
  bool WTF_is_pMany   = false;
  bool check_overlaps = false;

  return new G4PVPlacement{rotation   .value_or(nullptr),
                           position   .value_or(G4ThreeVector{}),
                           child      .value(),
                           the_name,
                           parent     .value_or(nullptr),
                           WTF_is_pMany,
                           copy_number.value_or(0),
                           check_overlaps};
}

} // namespace nain4

geometry_iterator begin(G4VPhysicalVolume& vol) { return geometry_iterator{&vol}; }
geometry_iterator   end(G4VPhysicalVolume&    ) { return geometry_iterator{    }; }
geometry_iterator begin(G4VPhysicalVolume* vol) { return begin(*vol); }
geometry_iterator   end(G4VPhysicalVolume* vol) { return   end(*vol); }
