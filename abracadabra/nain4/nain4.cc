#include "nain4.hh"

#include <G4PVPlacement.hh>
#include <G4String.hh>

#include <algorithm>
#include <iterator>

namespace nain4 {

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

std::vector<G4double> scale_by(G4double factor, std::initializer_list<G4double> const& data) {
  std::vector<G4double> out;
  out.reserve(data.size());
  std::transform(begin(data), end(data), back_inserter(out), [factor](auto d){ return d*factor; });
  return out;
}

// --------------------------------------------------------------------------------
// definition of material_properties
material_properties& material_properties::add(G4String const& key, vec const& energies, vec const& values) {
  table -> AddProperty(key, energies, values); // es-vs size equality assertion done in AddProperty
  return *this;
}

material_properties& material_properties::add(G4String const& key, vec const& energies, G4double   value ) {
  return add(key, energies, vec(energies.size(), value));
}

material_properties& material_properties::add_const(G4String const& key, G4double value) {
  table->AddConstProperty(key, value);
  return *this;
}

// --------------------------------------------------------------------------------
// stream redirection utilities

// redirect to arbitrary stream or buffer
redirect::redirect(std::ios& stream, std::streambuf* new_buffer)
: original_buffer(stream.rdbuf())
, stream(stream) {
  stream.rdbuf(new_buffer);
}

redirect::redirect(std::ios& stream, std::ios& new_stream) : redirect{stream, new_stream.rdbuf()} {}

redirect::~redirect() { stream.rdbuf(original_buffer); }

// redirect to /dev/null
silence::silence(std::ios& stream)
  : original_buffer{stream.rdbuf()}
  , stream{stream}
  , dev_null{"/dev/null"} {
  stream.rdbuf(dev_null.rdbuf());
}
silence::~silence() { stream.rdbuf(original_buffer); }


} // namespace nain4

geometry_iterator begin(G4VPhysicalVolume& vol) { return geometry_iterator{&vol}; }
geometry_iterator   end(G4VPhysicalVolume&    ) { return geometry_iterator{    }; }
geometry_iterator begin(G4VPhysicalVolume* vol) { return begin(*vol); }
geometry_iterator   end(G4VPhysicalVolume* vol) { return   end(*vol); }
