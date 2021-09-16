#include <geometries/inspect.hh>

#include <G4TransportationManager.hh>


world_geometry_inspector::world_geometry_inspector(G4RunManager* run_manager)
  : navigator{std::make_unique<G4Navigator>()}
{
  run_manager -> Initialize(); // ensure that geometry is closed
  G4VPhysicalVolume* world = G4TransportationManager::GetTransportationManager()
    -> GetNavigatorForTracking()
    -> GetWorldVolume();
  navigator -> SetWorldVolume(world);
}


G4double world_geometry_inspector::density_at(const G4ThreeVector& point) {
  auto touchable = std::make_unique<G4TouchableHistory>();
  navigator -> LocateGlobalPointAndUpdateTouchable(point, touchable.get(), false);
  auto physical_volume = touchable -> GetVolume();
  return physical_volume -> GetLogicalVolume() -> GetMaterial() -> GetDensity();
}
