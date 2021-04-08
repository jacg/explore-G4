#include "trajectory_map.hh"

#include <G4VTrajectory.hh>

std::map<int, G4VTrajectory*> trajectory_map::map_;


G4VTrajectory* trajectory_map::Get(int trackId) {
    std::map<int, G4VTrajectory*>::iterator it = map_.find(trackId);
    if (it == map_.end()) return 0;
    else return it->second;
}

void trajectory_map::Add(G4VTrajectory* trj) {
    map_[trj->GetTrackID()] = trj;
}

