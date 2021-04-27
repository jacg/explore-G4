#include "nain4.hh"

#include "geometries/imas.hh"
#include "geometries/nema.hh"
#include "geometries/sipm.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include <CLHEP/Units/PhysicalConstants.h>
#include <G4Types.hh>
#include <G4VPhysicalVolume.hh>

using nain4::volume;
using nain4::place;

auto a_nema_phantom() {
  // Use build_nema_phantom to create one realization of the cylyndrical NEMA
  // phantom pattern
  return build_nema_phantom{}
    .activity(1)
    .length(140*mm)
    .inner_radius(114.4*mm)
    .outer_radius(152.0*mm)
    .sphere(10*mm / 2, 2.8)
    .sphere(13*mm / 2, 2.8)
    .sphere(17*mm / 2, 2.8)
    .sphere(22*mm / 2, 2.8)
    .sphere(28*mm / 2, 0)
    .sphere(37*mm / 2, 0)
    .build();
}

G4VPhysicalVolume* square_array_of_sipms() {
  // 10 x 10 array array of SiPMs
  auto air = nain4::material("G4_AIR");
  auto sipm = sipm_hamamatsu_blue(true, nullptr);
  auto world = nain4::volume<G4Box>("world", air, 40*mm, 40*mm, 40*mm);
  for (int x=-35; x<35; x+=7) {
    for (int y=-35; y<35; y+=7) {
      nain4::place(sipm).in(world).at((x+3.5)*mm, (y+3.5)*mm, 30*mm).now();
    }
  }
  return nain4::place(world).now();

}

// clang-format off
G4double LXe_Scintillation(G4double energy) {
  using CLHEP::c_light;   using CLHEP::h_Planck;   using CLHEP::pi;
  // K. Fuji et al., "High accuracy measurement of the emission spectrum of liquid xenon
  // in the vacuum ultraviolet region",
  // Nuclear Instruments and Methods in Physics Research A 795 (2015) 293–297
  // http://ac.els-cdn.com/S016890021500724X/1-s2.0-S016890021500724X-main.pdf?_tid=83d56f0a-3aff-11e7-bf7d-00000aacb361&acdnat=1495025656_407067006589f99ae136ef18b8b35a04
  G4double lambda_peak  = 174.8 * nm;
  G4double lambda_FWHM  =  10.2 * nm;
  G4double lambda_sigma = lambda_FWHM / 2.35;

  G4double E_peak  = (h_Planck * c_light / lambda_peak);
  G4double E_sigma = (h_Planck * c_light * lambda_sigma / pow(lambda_peak, 2));

  G4double intensity = exp(-pow(E_peak / eV - energy / eV, 2) / (2 * pow(E_sigma / eV, 2)))
                     / (E_sigma / eV * sqrt(pi * 2.));

  return intensity;
}


G4double LXe_refractive_index(G4double energy) {
  // Formula for the refractive index taken from
  // A. Baldini et al., "Liquid Xe scintillation calorimetry
  // and Xe optical properties", arXiv:physics/0401072v1 [physics.ins-det]

  // The Lorentz-Lorenz equation (also known as Clausius-Mossotti equation)
  // relates the refractive index of a fluid with its density:
  // (n^2 - 1) / (n^2 + 2) = - A · d_M,     (1)
  // where n is the refractive index, d_M is the molar density and
  // A is the first refractivity viral coefficient:
  // A(E) = \sum_i^3 P_i / (E^2 - E_i^2),   (2)
  // with:
  G4double P[3] = {71.23, 77.75, 1384.89}; // [eV^2 cm3 / mole]
  G4double E[3] = { 8.4 ,  8.81,   13.2 }; // [eV]

  // Note.- Equation (1) has, actually, a sign difference with respect
  // to the one appearing in the reference. Otherwise, it yields values
  // for the refractive index below 1.

  // Leave G4's system of units, to avoid loss of numerical precision
  energy /= eV;

  // Calculate the virial coefficient.
  G4double virial = 0;
  for (G4int i=0; i<3; i++)
    virial += P[i] / (energy*energy - E[i]*E[i]); // eV²*cm3/mol/eV² = cm3/mol

  G4double mol_density =  2.953 / 131.29; // (g/cm3)/g*mol = mol/cm3
  G4double alpha = virial * mol_density; // (cm3/mol)*mol/cm3 = 1

  // Isolating now the n2 from equation (1) and taking the square root
  G4double n2 = (1 - 2*alpha) / (1 + alpha);

  if (n2 < 1) {
    n2 = 1;
    //throw "up (Non-physical refractive index)";
  }
  return sqrt(n2);
}

template<class F> auto map(F f, std::vector<double> const& xs) {
  std::vector<double> ys(xs.size());
  transform(begin(xs), end(xs), begin(ys), f);
  return ys;
}

template<class F> auto interpolate(F f, size_t N, double min, double max) {
  std::vector<double> xs(N);
  std::vector<double> ys(N);
  iota(begin(xs), end(xs), (max - min) / N);
  transform(begin(xs), end(xs), begin(ys), f);
  return make_tuple(xs, ys);
}

G4MaterialPropertiesTable* LXe_optical_material_properties() {
  /// The time constants are taken from E. Hogenbirk et al 2018 JINST 13 P10031
  G4double no_absorption = 1e8  * m; // approx. infinity
  G4double optphot_min_E = 1    * eV;
  G4double optphot_max_E = 8.21 * eV;
  std::function<double(double)> rindex = LXe_refractive_index;
  std::function<double(double)> scinty = LXe_Scintillation;

  // Sampling from ~151 nm to 200 nm <----> from 6.20625 eV to 8.21 eV // TODO convert here
  auto [sc_energies, sc_values] = interpolate(scinty, 500, 6.20625*eV   , optphot_max_E);
  auto [ri_energies, ri_values] = interpolate(rindex, 200, optphot_min_E, optphot_max_E);

  return n4::material_properties()
    .add("RINDEX", ri_energies, ri_values)
    .add("RESOLUTIONSCALE"   ,     1        )
    .add("YIELDRATIO"        ,     0.03     )
    .add("SCINTILLATIONYIELD", 58708   / MeV)
    .add("RAYLEIGH"          ,    36   * cm )
    .add("FASTTIMECONSTANT"  ,     2   * ns )
    .add("SLOWTIMECONSTANT"  ,    43.5 * ns )
    .add("ATTACHMENT"        ,  1000   * ms )
    .add("FASTCOMPONENT", sc_energies, sc_values)
    .add("SLOWCOMPONENT", sc_energies, sc_values)
    .add("ABSLENGTH", no_absorption)
    .done();

}

G4VPhysicalVolume* cylinder_lined_with_hamamatsus(double length, double radius) {
  // LXe-filled cylindrical shell, lined with hamamamtus
  auto air = nain4::material("G4_AIR");
  auto lXe = nain4::material("G4_lXe");
  lXe -> SetMaterialPropertiesTable(LXe_optical_material_properties());

  auto xenon    = volume<G4Tubs>("LXe"     , lXe, 0.0,     radius, length/2, 0.0, CLHEP::twopi);
  auto cavity   = volume<G4Tubs>("Cavity"  , air, 0.0, 0.9*radius, length/2, 0.0, CLHEP::twopi);
  auto envelope = volume<G4Box> ("Envelope", air,      1.1*radius, 1.1*radius, 1.1*length/2);

  line_cylinder_with_tiles(xenon, sipm_hamamatsu_blue(true, nullptr), 1*mm);
  place(cavity).in(xenon)   .now();
  place(xenon) .in(envelope).now();
  return place(envelope).now();
}
