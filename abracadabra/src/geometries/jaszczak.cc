#include "geometries/jaszczak.hh"

#include "nain4.hh"

using nain4::material;
using nain4::place;
using nain4::volume;

jaszczak_phantom build_jaszczak_phantom::build() {
  return std::move(*this);
}

G4PVPlacement* jaszczak_phantom::geometry() const {
  auto water = material("G4_WATER"); // The radioactive source is floating around in water
  auto pmma  = material("G4_WATER"); // TODO replace water with something appropriate

}
