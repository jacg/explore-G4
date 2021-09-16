#include <geometries/inspect.hh>
#include <G4TransportationManager.hh>

std::unique_ptr<G4Navigator> get_navigator() {
  auto navigator = std::make_unique<G4Navigator>();
  G4VPhysicalVolume* world = G4TransportationManager::GetTransportationManager()
    -> GetNavigatorForTracking()
    -> GetWorldVolume();
  navigator -> SetWorldVolume(world);
  return navigator;
}
