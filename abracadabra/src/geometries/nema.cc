#include "geometries/nema.hh"

#include "random/random.hh"
#include "utils/enumerate.hh"

#include "nain4.hh"

#include <G4Box.hh>
#include <G4Orb.hh>
#include <G4Tubs.hh>
#include <G4MultiUnion.hh>
#include <G4UnionSolid.hh>

#include <G4RandomDirection.hh>
#include <G4PVPlacement.hh>

#include <G4SystemOfUnits.hh>

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

using nain4::material;
using nain4::place;
using nain4::volume;

// ===== Section 3: Spatial Resolution =======================================================

nema_3_phantom::nema_3_phantom(G4double fov_length)
  : vertices{{0,  1*cm, 0},
             {0, 10*cm, 0},
             {0, 20*cm, 0},
             {0,  1*cm, fov_length * 3 / 8},
             {0, 10*cm, fov_length * 3 / 8},
             {0, 20*cm, fov_length * 3 / 8}}
{}

G4PVPlacement* nema_3_phantom::geometry() const {
  // ----- Materials --------------------------------------------------------------
  auto air = material("G4_AIR");

  // Find extent of bounding box to deduce envelope size.
  G4double x_max = 0, y_max = 0, z_max = 0;
  for (auto p: vertices) {
    x_max = std::max(x_max, std::abs(p.getX()));
    y_max = std::max(y_max, std::abs(p.getY()));
    z_max = std::max(z_max, std::abs(p.getZ()));
  }

  // Envelope size:
  // - 10% margin around content bounding box,
  // - minimum 1cm in any dimension
  // - must be centred at origin (when used as world) otherwise G4 crashes
  auto half_x = std::max(x_max, 9*mm) * 1.1;
  auto half_y = std::max(y_max, 9*mm) * 1.1;
  auto half_z = std::max(z_max, 9*mm) * 1.1;

  auto container = volume<G4Box>("Cylinder", air, half_x      , half_y      , half_z);
  auto envelope  = volume<G4Box>("Envelope", air, half_x * 1.1, half_y * 1.1, half_z * 1.1);

  // Indicate positions of point sources with finite spheres
  auto marker_radius = 10 * mm;
  for (auto [count, position]: enumerate(vertices)) {
    std::string name = "Source_" + std::to_string(count);
    auto ball  = volume<G4Orb>(name, air, marker_radius);
    place(ball).in(container).at(position).now();
  }
  place(container).in(envelope).now();
  return place(envelope).now();
}

// ===== Section 4: Scatter Fraction, Count Losses, and Randoms ============================

G4PVPlacement* nema_4_phantom::geometry() const {
  auto nist = G4NistManager::Instance();
  nist -> BuildMaterialWithNewDensity("NEMA4_POLYETHYLENE", "G4_POLYETHYLENE", 0.96 * g / cm3);
  auto air  = material("G4_AIR");
  auto poly = material("NEMA4_POLYETHYLENE");

  auto l = 700 * mm;
  auto half_l = l / 2;
  auto D = 203   * mm, R = D / 2;
  auto d =   3.2 * mm, r = d / 2;
  auto e_half_l = 1.1 * half_l + abs(z_offset);

  auto cylinder = volume<G4Tubs>("Cylinder", poly, 0.0, R, half_l, 0.0, 360*deg);
  auto source   = volume<G4Tubs>("Source"  , poly, 0.0, r, half_l, 0.0, 360*deg);
  auto envelope = volume<G4Box> ("Envelope", air , R * 1.1, R * 1.1, e_half_l);

  place(source)  .in(cylinder).at(0, y_offset, 0).now();
  place(cylinder).in(envelope).at(0, 0, z_offset).now();
  return place(envelope)                         .now();
}

G4ThreeVector nema_4_phantom::generate_vertex() const {
  auto z = uniform(-half_length, half_length) + z_offset;
  return {0, y_offset, z};
}


// ===== Section 7: Image Qualitiy, Accuracy of Corrections ==================================

build_nema_7_phantom& build_nema_7_phantom::sphereR(G4double radius, G4double activity) {
  spheres.emplace_back(radius, activity);
  return *this;
};


nema_7_phantom build_nema_7_phantom::build() {

  // We care about the relative, not absolute, volumes: ignore factors of pi
  G4double spheres_total_volume = 0;
  std::vector<G4double>     weights{};
  std::vector<G4double> sub_weights{};
  weights.reserve(spheres.size() + 1); // Extra element for phantom body
  for (auto& sphere : spheres) {
    auto r = sphere.radius;
    auto volume = r * r * r * 4/3;
    spheres_total_volume += volume;
    weights.push_back(volume * sphere.activity);
  }
  {
    auto l = lung_r;
    auto h = half_length * 2;
    auto [top_volume, corners_volume, base_volume] = sub_volumes();
    auto lung_volume = l * l * h;
    auto body_volume = top_volume + corners_volume + base_volume - spheres_total_volume - lung_volume;
    auto body_weight = body_volume * background;
    weights.push_back(body_weight);
    sub_weights = {top_volume, corners_volume/2, corners_volume/2, base_volume};
  }
  pick_region     = biased_choice(    weights);
  pick_sub_region = biased_choice(sub_weights);
  return std::move(*this);
}

G4ThreeVector nema_7_phantom::sphere_position(int n) const {
  auto angle = (n+1) * 360 * deg / spheres.size();
  auto x     = inner_r * cos(angle);
  auto y     = inner_r * sin(angle);
  return {x, y, 0};
}

G4PVPlacement* nema_7_phantom::geometry() const {
  // ----- Materials --------------------------------------------------------------
  auto air  = material("G4_AIR");
  auto lung = material("G4_AIR"); // TODO low atomic number material with density 0.3 ± 0.1 g/mL
  // TODO source and body materials should not be air
  auto body_material = air;

  auto pi = 180 * deg;

  auto z_offset = half_length - to_end; // NEMA requires shperes at 7cm from phantom end
  auto env_half_length = 1.1 * half_length + abs(z_offset);
  auto env_half_width  = 1.1 * top_r;

  auto vol_envelope = volume<G4Box> ("Envelope", air , env_half_width, env_half_width, env_half_length);
  auto vol_lung     = volume<G4Tubs>("Lung"    , lung, 0.0,  lung_r, half_length, 0.0, 2*pi);

  auto corner_c_x = top_r - corner_r;
  auto corner_c_y = - corner_c_x / 2;
  auto base_half_x = corner_c_x;
  auto base_half_y = corner_r / 2;
  auto top_half = new G4Tubs("Top"   , 0.0, top_r   , half_length,  0, pi);
  auto corner   = new G4Tubs("Corner", 0.0, corner_r, half_length, pi, pi);
  auto base     = new G4Box ("Base"  , base_half_x, base_half_y, half_length);

  // Using G4MultiUnion should be better than G4UnionSolid, but it seems broken

  // G4Transform3D top_half_trasform{{}, {        0  , corner_c_y              , 0}};
  // G4Transform3D  corner1_trasform{{}, {-corner_c_x, corner_c_y              , 0}};
  // G4Transform3D  corner2_trasform{{}, { corner_c_x, corner_c_y              , 0}};
  // G4Transform3D     base_trasform{{}, {        0  , corner_c_y - base_half_y, 0}};

  // auto body_solid = new G4MultiUnion("Body");
  // body_solid -> AddNode(*top_half, top_half_trasform);
  // body_solid -> AddNode(*corner  ,  corner1_trasform);
  // body_solid -> AddNode(*corner  ,  corner2_trasform);
  // body_solid -> AddNode(*base    ,     base_trasform);
  // body_solid -> Voxelize();
  // auto vol_body = new G4LogicalVolume(body_solid, body_material, "Body");

  auto no_rot = nullptr;
  auto union1     = new G4UnionSolid("union1", top_half, corner, no_rot, {-corner_c_x,          0  , 0});
  auto union2     = new G4UnionSolid("union2", union1  , corner, no_rot, { corner_c_x,          0  , 0});
  auto body_solid = new G4UnionSolid("Body"  , union2  , base  , no_rot, {        0  , -base_half_y, 0});

  auto vol_body = new G4LogicalVolume(body_solid, body_material, "Body");

  // Build and place spheres
  for (auto [count, sphere]: enumerate(spheres)) {
    std::string name = "Sphere_" + std::to_string(count);
    auto ball  = volume<G4Orb>(name, air, sphere.radius);
    auto position = sphere_position(count) + G4ThreeVector{0, -corner_c_y, z_offset};
    place(ball).in(vol_body).at(position).now();
  }

  // ----- Build geometry by organizing volumes in a hierarchy --------------------
  place(vol_body).in(vol_envelope).at(0,  corner_c_y, -z_offset).now();
  place(vol_lung).in(vol_body)    .at(0, -corner_c_y,  0       ).now();
  return place(vol_envelope).now();
}

void generate_back_to_back_511_keV_gammas(G4Event* event, G4ThreeVector position, G4double time) {
  auto gamma = nain4::find_particle("gamma");
  auto p = 511*keV * G4RandomDirection();
  auto vertex =      new G4PrimaryVertex(position, time);
  vertex->SetPrimary(new G4PrimaryParticle(gamma,  p.x(),  p.y(),  p.z()));
  vertex->SetPrimary(new G4PrimaryParticle(gamma, -p.x(), -p.y(), -p.z()));
  event -> AddPrimaryVertex(vertex);
}

G4ThreeVector nema_7_phantom::generate_vertex_in_body() const {
  auto corner_c_x = top_r - corner_r; // TODO avoid copy-paste from geometry()
  auto corner_c_y = corner_c_x / 2;

  auto z = uniform(0, 2*half_length) - 7*cm;
  auto region = pick_sub_region();

  if (region == 0) { // top
    auto [x, y] = random_on_disc(top_r);
    if (y < 0) { y = -y; }
    y -= corner_c_y;
    return {x,y,z};
  }

  if (region == 3) { // base
    auto base_half_width  = top_r - corner_r;
    auto base_full_height =         corner_r;
    auto x = uniform(-base_half_width, base_half_width);
    auto y = uniform(-base_full_height, 0) - corner_c_y;
    return {x,y,z};

  } else { // corners
    auto [x, y] = random_on_disc(corner_r);
    if (y > 0) { y = -y; }
    y -= corner_c_y;
    if (region==1) { if (x < 0) x = -x; x += corner_c_x; }
    else           { if (x > 0) x = -x; x -= corner_c_x; }
    return {x,y,z};
  }
}

G4ThreeVector nema_7_phantom::generate_vertex() const {
  G4ThreeVector offset; // TODO adjust for physical placement of logical geometry
  G4ThreeVector local_position;
  auto region = pick_region();
  if (region < spheres.size()) { // One of the spheres
    auto centre = sphere_position(region);
    local_position = centre + random_in_sphere(spheres[region].radius);
  } else { // The phantom's body
    do {
      local_position = generate_vertex_in_body();
    } while (inside_lung(local_position) || inside_a_sphere(local_position));
  }
  return local_position + offset; // TODO: rotation!
}

bool nema_7_phantom::inside_lung(G4ThreeVector& position) const {
  auto r2 = lung_r * lung_r;
  auto x = position.x();
  auto y = position.y();
  return x*x + y*y < r2;
}

bool nema_7_phantom::inside_this_sphere(size_t n, G4ThreeVector& position) const {
    auto r = spheres[n].radius;
    auto r2 = r*r;
    auto centre = sphere_position(n);
    return (position - centre).mag2() < r2;
}

bool nema_7_phantom::inside_a_sphere(G4ThreeVector& position) const {
  auto region = in_which_region(position);
  return region && region.value() < spheres.size();
}

std::optional<size_t> nema_7_phantom::in_which_region(G4ThreeVector& position) const {
  for (size_t n=0; n<spheres.size(); ++n) {
    if (inside_this_sphere(n, position)) { return n; }
  }
  if (inside_lung (position)) { return spheres.size(); }
  if (inside_whole(position)) { return spheres.size() + 1; }
  return {};

}
std::tuple<G4double, G4double, G4double> nema_7_phantom::sub_volumes() const {
  // All volumes scaled down by a factor of pi
  auto pi = 180 * deg;
  auto t =    top_r;
  auto c = corner_r;
  auto h = half_length * 2;
  auto     top_volume = t * t           * h / 2;
  auto corners_volume = c * c           * h / 2;
  auto    base_volume = 2 * c * (t - c) * h / pi; // compensate for missing pi in circular volumes
  return {top_volume, corners_volume, base_volume};
}
