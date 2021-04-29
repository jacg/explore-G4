#include "geometries/imas.hh"
#include "geometries/sipm.hh"
#include "materials/LXe.hh"

#include "io/hdf5.hh"
#include "nain4.hh"

#include <G4Box.hh>
#include <G4RotationMatrix.hh>
#include <G4ThreeVector.hh>
#include <G4Tubs.hh>
#include <G4Types.hh>
#include <G4SystemOfUnits.hh>

#include <initializer_list>
#include <tuple>

using CLHEP::pi;
using CLHEP::twopi;
using nain4::material;
using nain4::place;
using nain4::volume;
using std::make_tuple;


G4PVPlacement* imas_demonstrator(n4::sensitive_detector* sd) {

  // ----- Materials --------------------------------------------------------------
  auto air     = material("G4_AIR");
  auto steel   = material("G4_STAINLESS-STEEL");
  auto vacuum  = material("G4_Galactic");
  auto sensors = material("G4_WATER");    // TODO
  auto quartz  = material("G4_WATER");    // TODO
  auto LXe     = material("G4_lXe");
  auto housing = steel;

  LXe -> SetMaterialPropertiesTable(LXe_optical_material_properties());
  // ----- Dimensions -------------------------------------------------------------
  auto inner_radius = 325 * mm;

  // Thicknesses of each layer       // Outer radii of each layer
  auto dr_housing    =   3 * mm;     auto r_housing    = inner_radius + dr_housing;
  auto dr_vacuum_in  =  25 * mm;     auto r_vacuum_in  = r_housing    + dr_vacuum_in;
  auto dr_steel_in   =   3 * mm;     auto r_steel_in   = r_vacuum_in  + dr_steel_in;
  auto dr_LXe        =  40 * mm;     auto r_LXe        = r_steel_in   + dr_LXe;
  auto dr_quartz     =  20 * mm;     auto r_quartz     = r_LXe        + dr_quartz;
  auto dr_sensors    =   3 * mm;     auto r_sensors    = r_quartz     + dr_sensors;
  auto dr_vacuum_out = 200 * mm;     auto r_vacuum_out = r_sensors    + dr_vacuum_out;
  auto dr_steel_out  =   5 * mm;     auto r_steel_out  = r_vacuum_out + dr_steel_out;

  auto length = 70 * cm, half_length = length / 2;

  auto envelope_length = 1.1 * length;
  auto envelope_width  = 1.1 * r_steel_out;

  // ----- Logical volumes making up the geometry ---------------------------------

  // Bind invariant args (3, 5, 6 and 7) of volume
  auto vol = [half_length](auto name, auto material, auto radius) {
    return volume<G4Tubs>(name, material, 0.0, radius, half_length, 0.0, twopi);
  };

  auto vol_cavity     = vol("Cavity"      , air    , inner_radius);
  auto vol_housing    = vol("Housing"     , housing, r_housing   );
  auto vol_vacuum_in  = vol("Inner_vacuum", vacuum , r_vacuum_in );
  auto vol_steel_in   = vol("Inner_steel" , steel  , r_steel_in  );
  auto vol_LXe        = vol("LXe"         , LXe    , r_LXe       );
  auto vol_quartz     = vol("Quartz"      , quartz , r_quartz    );
  auto vol_sensors    = vol("Sensors"     , sensors, r_sensors   );
  auto vol_vacuum_out = vol("Outer_vacuum", vacuum , r_vacuum_out);
  auto vol_steel_out  = vol("Outer_steel" , steel  , r_steel_out );
  auto vol_envelope = volume<G4Box>("Envelope", air, envelope_width, envelope_width, envelope_length);

  // TODO world volume ?

  // ----- Build geometry by organizing volumes in a hierarchy --------------------
  place(vol_cavity    ).in(vol_housing   ).now();
  place(vol_housing   ).in(vol_vacuum_in ).now();
  place(vol_vacuum_in ).in(vol_steel_in  ).now();
  place(vol_steel_in  ).in(vol_LXe       ).now();
  place(vol_LXe       ).in(vol_quartz    ).now();
  place(vol_quartz    ).in(vol_sensors   ).now();
  place(vol_sensors   ).in(vol_vacuum_out).now();
  place(vol_vacuum_out).in(vol_steel_out ).now();
  place(vol_steel_out ).in(vol_envelope  ).now();

  line_cylinder_with_tiles(vol_sensors, sipm_hamamatsu_blue(true, sd), 1*mm);

  return place(vol_envelope).now();
}

auto box_dimensions(G4LogicalVolume* tile) {
  auto box = dynamic_cast<G4Box*>(tile -> GetSolid());
  if (!tile) { throw "SiPM should be a G4Box"; }
  auto dx = box -> GetXHalfLength() * 2;
  auto dy = box -> GetYHalfLength() * 2;
  auto dz = box -> GetZHalfLength() * 2;
  return make_tuple(dx, dy, dz);
}

auto axial_positioning(double gap, double length, double dx) {
  auto pitch    = dx + gap;
  auto N        = floor((length - gap) / pitch);
  auto z_margin = (length - (N * pitch) + gap) / 2;
  auto first_z = -(length - dx) / 2 + z_margin;
  struct z { size_t N; double first; double pitch; };
  return z{(size_t)N, first_z, pitch};
}

auto angular_positioning(double outer_r, double dz, double pitch) {
  auto reduced_r = outer_r - dz;
  auto circumference = twopi * reduced_r;
  auto N_phi = (size_t)floor(circumference / pitch);
  auto d_phi = twopi / N_phi;
  struct phi { double r; size_t N; double delta; };
  return phi{reduced_r, N_phi, d_phi};
}

// TODO: this is not adequately tested
void line_cylinder_with_tiles(G4LogicalVolume* cylinder, G4LogicalVolume* tile, G4double gap) {

  // Cylinder dimensions
  auto tub = dynamic_cast<G4Tubs*>(cylinder -> GetSolid());
  if (!tub) { throw "Cylinder should be a G4Tubs"; }
  auto outer_r = tub -> GetRMax();
  auto length  = tub -> GetDz() * 2;

  // Tile dimensions
  auto [dx, dy, dz] = box_dimensions(tile);
  if (dx != dy) { throw "The tile should have equal x and y dimensions"; }

  // Tile positions
  auto z   =   axial_positioning(gap, length, dx);
  auto phi = angular_positioning(outer_r, dz, z.pitch);

  //return;
  G4ThreeVector x_axis{1, 0, 0};
  G4ThreeVector z_axis{0, 0, 1};
  G4RotationMatrix around_x_axis_90_degrees{x_axis, -pi / 2};
  auto const situate_sipm = place(tile).in(cylinder)
    .rotate(around_x_axis_90_degrees) // active region facing inwards
    .at(0, phi.r, 0);                 // set distance now; rotation in phi comes later

  size_t N_copy = 0;
  for (size_t n_phi = 0; n_phi<phi.N; ++n_phi) {
    G4RotationMatrix around_z_axis_phi{z_axis, n_phi*phi.delta};
    for (size_t nz=0; nz<z.N; ++nz) {
      auto zz = z.first + nz * z.pitch;
      situate_sipm.clone()
        .at(0, 0, zz)
        .rotate(around_z_axis_phi)
        .copy_no(N_copy)
        .now();
    }
    ++N_copy;
  }
}
