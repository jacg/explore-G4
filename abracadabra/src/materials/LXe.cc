#include <materials/LXe.hh>

#include <nain4.hh>
#include <utils/interpolate.hh>

#include <G4SystemOfUnits.hh>


namespace nexus_LXe { G4MaterialPropertiesTable* OpticalMaterialProperties(); }

G4Material* air_with_properties() {
  auto air = n4::material("G4_air");
  auto air_properties = n4::material_properties()
    .add("RINDEX", 1.002)
    .add("ABSLENGTH", 1e8 * m)
    .done();
  air -> SetMaterialPropertiesTable(LXe_optical_material_properties());
  //air -> SetMaterialPropertiesTable(air_properties);
  return air;
}

G4Material* LXe_with_properties() {
  auto LXe = n4::material("G4_lXe");
  LXe -> SetMaterialPropertiesTable(LXe_optical_material_properties());
  return LXe;
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

G4MaterialPropertiesTable* LXe_optical_material_properties() {
  /// The time constants are taken from E. Hogenbirk et al 2018 JINST 13 P10031
  G4double no_absorption = 1e8  * m; // approx. infinity
  G4double optphot_min_E = 1    * eV;
  G4double optphot_max_E = 8.21 * eV;

  // Sampling from ~151 nm to 200 nm <----> from 6.20625 eV to 8.21 eV // TODO convert here
  auto [sc_energies, sc_values] = interpolate(LXe_Scintillation   , 500, 6.20625*eV   , optphot_max_E);
  auto [ri_energies, ri_values] = interpolate(LXe_refractive_index, 200, optphot_min_E, optphot_max_E);

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
    .add("ABSLENGTH", {optphot_min_E, optphot_max_E}, {no_absorption, no_absorption})
    .done();
}

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

// ====================================================================================================
// ====================================================================================================
// ====================================================================================================
// ----------------------------------------------------------------------------


#include <G4AnalyticalPolSolver.hh>
#include <G4MaterialPropertiesTable.hh>

#include "CLHEP/Units/SystemOfUnits.h"
#include "CLHEP/Units/PhysicalConstants.h"
#include <stdexcept>

namespace nexus_LXe {

  using namespace CLHEP;

  G4double Density() {
    // Density at 1 atm, T ~ 160 K
    auto density_ = 2.953 * g/cm3;
    return density_;
  }

  G4double RefractiveIndex(G4double energy) {
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
    G4double E[3] = {8.4, 8.81, 13.2};       // [eV]

    // Note.- Equation (1) has, actually, a sign difference with respect
    // to the one appearing in the reference. Otherwise, it yields values
    // for the refractive index below 1.

    // Let's calculate the virial coefficient.

    // We won't use the implicit system of units of Geant4 because
    // it results in loss of numerical precision.

    energy = energy / eV;
    G4double virial = 0.;

    for (G4int i=0; i<3; i++)
      virial = virial + P[i] / (energy*energy - E[i]*E[i]); // eV²*cm3/mol/eV² = cm3/mol

    G4double mol_density =  2.953  / 131.29; // (g/cm3)/g*mol = mol/cm3
    G4double alpha = virial * mol_density; // (cm3/mol)*mol/cm3 = 1

    // Isolating now the n2 from equation (1) and taking the square root
    G4double n2 = (1. - 2*alpha) / (1. + alpha);

    if (n2 < 1.) {
      G4String msg = "Non-physical refractive index for energy. Use n=1 instead. ";
	// + bhep::to_string(energy) + " eV. Use n=1 instead.";
      G4cout << "refractive index for energy " << energy << " = " << n2 << G4endl;
      G4Exception("[XenonLiquidProperties]", "RefractiveIndex()",
      	 	  JustWarning, msg);
      n2 = 1.;
    }
    return sqrt(n2);
  }

  G4double Scintillation(G4double energy) {
    // K. Fuji et al., "High accuracy measurement of the emission spectrum of liquid xenon
    // in the vacuum ultraviolet region",
    // Nuclear Instruments and Methods in Physics Research A 795 (2015) 293–297
    // http://ac.els-cdn.com/S016890021500724X/1-s2.0-S016890021500724X-main.pdf?_tid=83d56f0a-3aff-11e7-bf7d-00000aacb361&acdnat=1495025656_407067006589f99ae136ef18b8b35a04
    G4double Wavelength_peak = 174.8*nm;
    G4double Wavelength_FWHM = 10.2*nm;
    G4double Wavelength_sigma = Wavelength_FWHM/2.35;

    G4double Energy_peak = (h_Planck*c_light/Wavelength_peak);
    G4double Energy_sigma = (h_Planck*c_light*Wavelength_sigma/pow(Wavelength_peak,2));
    // G4double bin = 6*Energy_sigma/500;

    G4double intensity =
	  exp(-pow(Energy_peak/eV-energy/eV,2)/(2*pow(Energy_sigma/eV, 2)))/(Energy_sigma/eV*sqrt(pi*2.));

    return intensity;
  }

  void Scintillation(std::vector<G4double>& energy, std::vector<G4double>& intensity) {
    for (unsigned i=0; i<energy.size(); i++)
      intensity.push_back(Scintillation(energy[i]));
  }


  // class XenonLiquidProperties  {
  // public:
  //   XenonLiquidProperties();
  //   ~XenonLiquidProperties();
  //   G4double Density();
  //   G4double RefractiveIndex(G4double energy);
  //   G4double Scintillation(G4double energy);
  //   void Scintillation(std::vector<G4double>& energy, std::vector<G4double>& intensity);

  // private:
  //   G4double density_;

  // };

  static constexpr G4double optPhotMinE_ = 1.   * eV;
  static constexpr G4double optPhotMaxE_ = 8.21 * eV;
  static constexpr G4double noAbsLength_ = 1.e8  * m;

G4MaterialPropertiesTable* OpticalMaterialProperties() {
  /// The time constants are taken from E. Hogenbirk et al 2018 JINST 13 P10031
  G4MaterialPropertiesTable* LXe_mpt = new G4MaterialPropertiesTable();

  const G4int ri_entries = 200;
  G4double eWidth = (optPhotMaxE_ - optPhotMinE_) / ri_entries;

  std::vector<G4double> ri_energy;
  for (int i=0; i<ri_entries; i++) {
    ri_energy.push_back(optPhotMinE_ + i * eWidth);
  }

  std::vector<G4double> ri_index;

  for (G4int i=0; i<ri_entries; i++) {
    ri_index.push_back(RefractiveIndex(ri_energy[i]));
  }

  assert(ri_energy.size() == ri_index.size());
  LXe_mpt->AddProperty("RINDEX", ri_energy.data(), ri_index.data(), ri_energy.size());

  // for (G4int i=ri_entries-1; i>=0; i--) {
  //   G4cout << h_Planck*c_light/ri_energy[i]/nanometer << " nm, " << rindex[i] << G4endl;
  // }

  // Sampling from ~151 nm to 200 nm <----> from 6.20625 eV to 8.21 eV
  const G4int sc_entries = 500;
  const G4double minE = 6.20625*eV;
  eWidth = (optPhotMaxE_ - minE) / sc_entries;

  std::vector<G4double> sc_energy;
  for (int j=0; j<sc_entries; j++){
    sc_energy.push_back(minE + j * eWidth);
  }
  std::vector<G4double> intensity;
  Scintillation(sc_energy, intensity);

  assert(sc_energy.size() == intensity.size());
  LXe_mpt->AddProperty("FASTCOMPONENT", sc_energy.data(), intensity.data(), sc_energy.size());
  LXe_mpt->AddProperty("SLOWCOMPONENT", sc_energy.data(), intensity.data(), sc_energy.size());

  LXe_mpt->AddConstProperty("SCINTILLATIONYIELD", 58708./MeV);
  LXe_mpt->AddConstProperty("RESOLUTIONSCALE", 1);
  LXe_mpt->AddConstProperty("RAYLEIGH", 36.*cm);
  LXe_mpt->AddConstProperty("FASTTIMECONSTANT", 2.*ns);
  LXe_mpt->AddConstProperty("SLOWTIMECONSTANT", 43.5*ns);
  LXe_mpt->AddConstProperty("YIELDRATIO", 0.03);
  LXe_mpt->AddConstProperty("ATTACHMENT", 1000.*ms);

  std::vector<G4double> abs_energy = {optPhotMinE_, optPhotMaxE_};
  std::vector<G4double> abs_length = {noAbsLength_, noAbsLength_};

  assert(abs_energy.size() == abs_length.size());
  LXe_mpt->AddProperty("ABSLENGTH", abs_energy.data(), abs_length.data(), abs_energy.size());

  return LXe_mpt;
}




} // end namespace nexus
