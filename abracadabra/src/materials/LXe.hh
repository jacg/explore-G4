#include <G4Material.hh>
#include <G4MaterialPropertiesTable.hh>

G4double LXe_refractive_index(G4double energy);
G4MaterialPropertiesTable* LXe_optical_material_properties();
G4double LXe_Scintillation(G4double energy);

G4Material*    LXe_with_properties();
G4Material* G4_LXe_with_properties();
G4Material*    air_with_properties();
G4Material* quartz_with_properties();

namespace nexus_LXe {

  G4double Density();
  G4double RefractiveIndex(G4double energy);
  G4double Scintillation(G4double energy);
  void Scintillation(std::vector<G4double>& energy, std::vector<G4double>& intensity);
  G4MaterialPropertiesTable* OpticalMaterialProperties();

}
