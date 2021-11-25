#include "geometries/jaszczak.hh"
#include "random/random.hh"
#include "utils/enumerate.hh"

#include "nain4.hh"

#include <G4Box.hh>
#include <G4Orb.hh>
#include <G4Tubs.hh>

#include <algorithm>

using nain4::material;
using nain4::place;
using nain4::volume;

using CLHEP::pi; using CLHEP::twopi;

jaszczak_phantom build_jaszczak_phantom::build() {
  // Scale relative activities so that the maximum one equals 1
  auto max_activity = std::max(activity_sphere, std::max(activity_body, activity_rod));
  activity_sphere /= max_activity;
  activity_body   /= max_activity;
  activity_rod    /= max_activity;
  std::cout << "Relative activities of Jaszczak components:\n";
  std::cout << "   sphere: " << activity_sphere << std::endl;
  std::cout << "   body  : " << activity_body   << std::endl;
  std::cout << "   rod   : " << activity_rod    << std::endl;
  return std::move(*this);
}


G4PVPlacement* jaszczak_phantom::geometry() const {
  auto air   = material("G4_AIR");
  auto water = material("G4_WATER"); // The radioactive source is floating around in water
  auto pmma  = material("G4_PLEXIGLASS");

  if (evacuate) {
    auto vacuum = material("G4_Galactic");
    air   = vacuum;
    water = vacuum;
    pmma  = vacuum;
  }

  auto env_half_length = height_body * 1.1;
  auto env_half_width  = radius_body * 1.1;

  auto body     = volume<G4Tubs>("Body"    , water, 0.0, radius_body, height_body/2, 0.0, twopi);
  auto envelope = volume<G4Box> ("Envelope", air , env_half_width, env_half_width, env_half_length);

  // Rods
  for (const auto [n, r] : enumerate(radii_rods)) { rod_sector(n, r, body, pmma); }

  // Spheres
  for (const auto [n, r] : enumerate(radii_spheres)) {
    auto name = "Sphere-" + std::to_string(n);
    auto ball = volume<G4Orb>(name, pmma, r);
    auto angle = (60 * deg) * n;
    auto x = radius_body / 2 * cos(angle);
    auto y = radius_body / 2 * sin(angle);
    auto z = height_spheres - (height_body / 2);
    place(ball).in(body).at(x,y,z).now();
  }

  place(body).in(envelope).now();
  return place(envelope).now();
}


void jaszczak_phantom::rod_sector(unsigned long n, G4double r,
                                  G4LogicalVolume* body, G4Material* material) const {
  auto d = 2 * r;
  auto z = (height_rods - height_body) / 2;
  G4RotationMatrix around_z_axis{{0,0,1}, n*pi/3};

  // Sector displacement from centre, to accommodate gap between sectors
  auto dx = gap * cos(pi/6);
  auto dy = gap * sin(pi/6);
  // Displacement of first rod WRT sector corner
  dx += r * sqrt(3);
  dy += r;
  // Basis vectors of rod lattice
  const auto Ax = 2.0, Ay = 0.0;
  const auto Bx = 1.0, By = sqrt(3);
  auto a = 0;
  for (bool did_b=true ; did_b; a+=1) {
    did_b = false;
    for (auto b = 0; /*break in body*/; b+=1, did_b = true) {
      auto x = (a*Ax + b*Bx) * d + dx;
      auto y = (a*Ay + b*By) * d + dy;
      if (sqrt(x*x + y*y) + r + margin >= radius_body) { break; }
      auto label = std::string("Rod-") + std::to_string(n);
      auto rod = volume<G4Tubs>(label, material, 0.0, r, height_rods/2, 0.0, twopi);
      place(rod).in(body).at(x,y,z).rotate(around_z_axis).now();
    }
  }
}

bool startswith(std::string const& text, char const * const start) {
  return ! text.rfind(start, 0);
}


// using T = std::map<std::string, unsigned>;
// void report(T& kept, T& rejected, std::string const& key) {
//   auto k = (float)kept[key];
//   auto r =    rejected[key];
//   auto t = k + r;
//   auto f = t ? k / t : 0;
//   std::cout << key << ": " << k << " / " << t << " = " << f << std::endl;
// }

#define MAYBE_GENERATE_IN(PATTERN, THRESHOLD)   \
  if (startswith(name, PATTERN)) {              \
    if (uniform() < THRESHOLD) {keep  [PATTERN] += 1; return point;}    \
    else                       {reject[PATTERN] += 1; continue;}        \
  }                                                                     \

// TODO This currently has no automated test. Idea: once the Jaszczak FOMs are
// written:
// 1. Run magic detector on jaszcak
// 2. Run imageprimaries on the output
// 3. Use FOMs (or something related to them) to check activity distribution
// But this approach is rather expensive. Perhaps a cheaper alternative can be found.
G4ThreeVector jaszczak_phantom::generate_vertex() const {
  static std::map<std::string, unsigned> keep   {{"Body", 0}, {"Sphere", 0}, {"Rod", 0}};
  static std::map<std::string, unsigned> reject {{"Body", 0}, {"Sphere", 0}, {"Rod", 0}};
  for (;;) {

    // report(keep, reject, "Body");
    // report(keep, reject, "Sphere");
    // report(keep, reject, "Rod");

    auto z = uniform(-height_body/2, height_body/2);
    auto [x,y] = random_on_disc(radius_body);
    G4ThreeVector point{x,y,z};

    auto name = inspector() -> volume_at(point) -> GetName();

    MAYBE_GENERATE_IN("Body"  , activity_body)
    MAYBE_GENERATE_IN("Rod"   , activity_rod)
    MAYBE_GENERATE_IN("Sphere", activity_sphere)
  }
}
#undef MAYBE_GENERATE_IN

world_geometry_inspector* jaszczak_phantom::inspector() const {
  if (! inspector_) {
    inspector_.reset(new world_geometry_inspector{run_manager -> get()});
  }
  return inspector_.get();
}
