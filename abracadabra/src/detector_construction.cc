#include "detector_construction.hh"

#include <G4Box.hh>
#include <G4Cons.hh>
#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4NistManager.hh>
#include <G4Orb.hh>
#include <G4PVPlacement.hh>
#include <G4RunManager.hh>
#include <G4Sphere.hh>
#include <G4String.hh>
#include <G4SystemOfUnits.hh>
#include <G4ThreeVector.hh>
#include <G4Trd.hh>


#include <utility>

// Create logical volume from solid and material
template<class SOLID, class NAME, class... ArgTypes>
G4LogicalVolume* logical(NAME name, G4Material* material, ArgTypes&&... args) {
  auto solid = new SOLID{std::forward<NAME>(name), std::forward<ArgTypes>(args)...};
  return new G4LogicalVolume{solid, material, solid->GetName()};
}

// Utility for concisely creating materials from NIST code
G4Material* material(G4String const& name) { return G4NistManager::Instance()->FindOrBuildMaterial(name); };

// ================================================================================
#include <optional>
using std::nullopt;
using std::optional;
using std::make_optional;

class place {
public:
  place(G4LogicalVolume* child)
  : child(child ? make_optional(child) : nullopt)
  , copy_number(0){}

  place(place const&) = default;

  place& at(double x, double y, double z) { return this->at({x, y, z}); }

  place& at(G4ThreeVector position_) {
    auto xxx = make_optional(position_);
    this->position.swap(xxx);
    return *this;
  }

  place& in(G4LogicalVolume* parent_) {
    auto xxx = make_optional(parent_);
    this->parent.swap(xxx);
    return *this;
  }

  place& id(unsigned id) {
    this->copy_number = id;
    return *this;
  }

  G4PVPlacement* now() { return this->operator()(); }

  G4PVPlacement* operator()() {
    // ----- Name --------------------------------------------------
    // + By default, the name is copied from the child volume.
    // + If a copy_number is specified, it is appended to the name.
    // + All of this is overriden if a name is provided explicitly.
    G4String the_name;
    if (this->name) {
      the_name = this -> name.value();
    } else {
      the_name = this -> child.value() -> GetName();
      if (this -> copy_number) {
        auto suffix = "-" + std::to_string(copy_number.value());
        the_name += suffix;
      }
    }
    // TODO: Think about these later
    bool WTF_is_pMany = false;
    bool check_overlaps = true;

    return new G4PVPlacement {
      rotation   .value_or(nullptr),
      position   .value_or(G4ThreeVector{}),
      child      .value(),
      the_name,
      parent     .value_or(nullptr),
      WTF_is_pMany,
      copy_number.value_or(0),
      check_overlaps
    };
  }

private:
  optional<G4LogicalVolume*>  child;
  optional<G4LogicalVolume*>  parent;
  optional<G4ThreeVector>     position;
  optional<G4RotationMatrix*> rotation;
  optional<G4String>          name;
  optional<int>               copy_number;
};

// ================================================================================

G4VPhysicalVolume* detector_construction::Construct() {

  // ----- Materials --------------------------------------------------------------
  auto air    = material("G4_AIR");
  auto water  = material("G4_WATER");
  auto tissue = material("G4_A-150_TISSUE");
  auto bone   = material("G4_BONE_COMPACT_ICRU");

  // ----- Dimensions -------------------------------------------------------------
  // Size of the detector
  auto length_xy = 20 * cm;
  auto length_z  = 30 * cm;

  // Envelope: G4Box requires half-lengths
  auto e_xy = 0.5 * length_xy;
  auto e_z  = 0.5 * length_z;

  // World volume needs a margin around everything inside it
  auto w_xy = 1.2 * e_xy;
  auto w_z  = 1.2 * e_z;

  // Trapezoid ---------------------------------------------------------------------
  auto t_dxa = 12 * cm / 2, t_dxb = 12 * cm / 2;
  auto t_dya = 10 * cm / 2, t_dyb = 16 * cm / 2;
  auto t_dz  =  6 * cm / 2;

  // Cone --------------------------------------------------------------------------
  auto c_rmin_a  = 0 * cm,   c_rmax_a = 2 * cm;
  auto c_rmin_b  = 0 * cm,   c_rmax_b = 4 * cm;
  auto c_hz      = 3 * cm;
  auto c_phi_min = 0 * deg,  c_phi_max = 360 * deg;

  // ----- Create the components of the detector ------------------------------------
  auto world     = logical<G4Box> ("World"        , air   , w_xy, w_xy, w_z);
  auto envelope  = logical<G4Box> ("Envelope"     , water , e_xy, e_xy, e_z);
  auto trapezoid = logical<G4Trd> ("BoneTrapezoid", bone  , t_dxa, t_dxb, t_dya, t_dyb, t_dz);
  auto cone      = logical<G4Cons>("TissueCone"   , tissue, c_rmin_a, c_rmax_a, c_rmin_b, c_rmax_b, c_hz, c_phi_min, c_phi_max);

  this->scoring_volume = trapezoid;

  // ----- Combine the components ---------------------------------------------------
  place(trapezoid).in(envelope).at(0, -1*cm, 7*cm).now();
  place(cone     ).in(envelope).at(0,  2*cm,-7*cm).now();
  place(envelope ).in(world   )                   .now();

  return place(world).now();
}
