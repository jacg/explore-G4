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
  G4VPhysicalVolume const* volume_at(const G4ThreeVector&);
  G4Material        const* material_at(const G4ThreeVector&);
private:
  std::unique_ptr<G4Navigator> navigator;
};
