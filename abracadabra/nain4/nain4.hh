#ifndef nain4_hh
#define nain4_hh

#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4PVPlacement.hh>
#include <G4String.hh>
#include <G4ThreeVector.hh>

#include <string>
#include <utility>
#include <vector>
#include <tuple>
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

// --------------------------------------------------------------------------------
// Utilies for concisely finding materials and elements

G4Material* material(G4String const& name);
G4Element * element (G4String const& name);

// The G4Material::AddElement is overloaded on double/int in the second
// parameter. Template argument deduction doesn't seem to be able to resolve
// this, when the values are nested inside an std::initializer_list argument.
// This forces the caller to specify the template argument explicitly, so we
// provide wrappers (material_from_elements_N and material_from_elements_F) with
// the hope that it's a slightly nicer interface.
template<typename NUMBER>
G4Material* material_from_elements(std::string name, G4double density, G4State state,
                                   std::vector<std::tuple<std::string, NUMBER>> components,
                                   bool warn = false)
{
  auto the_material = G4Material::GetMaterial(name, warn);
  if (!the_material) {
    auto n_components = static_cast<G4int>(components.size());
    the_material = new G4Material{name, density, n_components, state};
    for (auto [the_element, n_atoms]: components) {
      the_material -> AddElement(element(the_element), n_atoms);
    }
  }
  return the_material;
}


// Wrapper for material_from_elements<G4int>
inline
G4Material* material_from_elements_N(std::string name, G4double density, G4State state,
                                     std::vector<std::tuple<std::string, G4int>> components,
                                     bool warn = false) {
  return material_from_elements<G4int>(name, density, state, components, warn);
}

// Wrapper for material_from_elements<G4double>
inline
G4Material* material_from_elements_F(std::string name, G4double density, G4State state,
                                     std::vector<std::tuple<std::string, G4double>> components,
                                     bool warn = false) {
  return material_from_elements<G4double>(name, density, state, components, warn);
}

// --------------------------------------------------------------------------------

class place {
public:
  place(G4LogicalVolume* child)  : child(child ? make_optional(child) : nullopt) {}
  place(place const&) = default;

  place& at(double x, double y, double z)                 { return at({x, y, z}); }
  place& at(G4ThreeVector    pos)          { position    = pos    ; return *this; }
  place& id(int              id)           { copy_number = id     ; return *this; }
  place& in(G4LogicalVolume* parent_)      { parent      = parent_; return *this; }
  place& name(G4String       label_)       { label       = label_ ; return *this; }

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

} // namespace nain4

// --------------------------------------------------------------------------------

#include <iterator>
#include <queue>

// TODO Move into nain4 namespace
// TODO This is breadth-first; how about depth-first?
// TODO This is an input iterator; how about output/forward?
// TODO Swich to C++ 20 and do it with concepts
class geometry_iterator {
public:
  geometry_iterator() {}
  geometry_iterator(G4VPhysicalVolume* v) { this->q.push(v); }
  geometry_iterator(geometry_iterator const &) = default;
  geometry_iterator(geometry_iterator      &&) = default;

  using iterator_category = std::input_iterator_tag;
  using value_type        = G4VPhysicalVolume;
  using pointer           = value_type*;
  using reference         = value_type&;
  using difference_type   = std::ptrdiff_t;

  geometry_iterator  operator++(int) { auto tmp = *this; ++(*this); return tmp; }
  geometry_iterator& operator++(   ) {
    if (!this->q.empty()) {
      auto current = this->q.front();
      this->q.pop();
      auto logical = current->GetLogicalVolume();
      for(size_t d=0; d<logical->GetNoDaughters(); ++d) {
        this->q.push(logical->GetDaughter(d));
      }
    }
    return *this;
  }

  pointer   operator->()       { return  this->q.front(); }
  reference operator* () const { return *this->q.front(); }

  friend bool operator== (const geometry_iterator& a, const geometry_iterator& b) { return a.q == b.q; };
  friend bool operator!= (const geometry_iterator& a, const geometry_iterator& b) { return a.q != b.q; };

private:
  std::queue<G4VPhysicalVolume*> q{};
};

// By overloading begin and end, we can make G4PhysicalVolume
// usable with the STL algorithms and range-based for loops.
geometry_iterator begin(G4VPhysicalVolume&);
geometry_iterator   end(G4VPhysicalVolume&);
geometry_iterator begin(G4VPhysicalVolume*);
geometry_iterator   end(G4VPhysicalVolume*);



#endif
