#include <G4Navigator.hh>
#include <G4RunManager.hh>
#include <G4ThreeVector.hh>

#include <memory>
#include <functional>

class world_geometry_inspector {
public:
  /// run manager will be used to:
  /// 1. ensure that geometry is closed (by calling Initialize())
  /// 2. discover the world volume
  world_geometry_inspector(G4RunManager*);
  G4VPhysicalVolume const* volume_at(const G4ThreeVector&)   const;
  G4Material        const* material_at(const G4ThreeVector&) const;
private:
  std::unique_ptr<G4Navigator> navigator;
};

void attenuation_map(std::tuple<float, float, float> fov_full_size,
                     std::tuple<unsigned short, unsigned short, unsigned short> n_voxels,
                     std::string filename,
                     world_geometry_inspector& inspect);
