#ifndef nain4_hh
#define nain4_hh

#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4PVPlacement.hh>
#include <G4String.hh>
#include <G4ThreeVector.hh>

#include <utility>
#include <optional>

namespace nain4 {

using std::make_optional;
using std::nullopt;
using std::optional;

// Create logical volume from solid and material
template<class SOLID, class NAME, class... ArgTypes>
G4LogicalVolume* volume(NAME name, G4Material* material, ArgTypes&&... args) {
  auto solid = new SOLID{std::forward<NAME>(name), std::forward<ArgTypes>(args)...};
  return new G4LogicalVolume{solid, material, solid->GetName()};
}

// Utility for concisely creating materials from NIST code
G4Material* material(G4String const& name);

// ================================================================================

class place {
public:
  place(G4LogicalVolume* child)  : child(child ? make_optional(child) : nullopt) {}
  place(place const&) = default;

  place& at(double x, double y, double z)                 { return at({x, y, z}); }
  place& at(G4ThreeVector pos)           { position.emplace(pos)  ; return *this; }
  place& id(int id)                      { copy_number    = id    ; return *this; }
  place& in(G4LogicalVolume* parent_)    { parent.emplace(parent_); return *this; }
  place& name(G4String label_)           { label .emplace(label_ ); return *this; }

  G4PVPlacement* operator()()                                     { return now(); }
  G4PVPlacement* now();

private:
  optional<G4LogicalVolume*>  child;
  optional<G4LogicalVolume*>  parent;
  optional<G4ThreeVector>     position;
  optional<G4RotationMatrix*> rotation;
  optional<G4String>          label;
  optional<int>               copy_number;
};

}

#endif
