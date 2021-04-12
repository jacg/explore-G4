#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <G4VTrajectory.hh>
#include <G4Allocator.hh>
#include <G4ParticleDefinition.hh>

class G4Track;
class G4ParticleDefinition;
class G4VTrajectoryPoint;

typedef std::vector<G4VTrajectoryPoint*> TrajectoryPointContainer;

class trajectory: public G4VTrajectory {
public:
    trajectory(const G4Track*);
    trajectory(const trajectory&);
    virtual ~trajectory();

    void* operator new(size_t);
    void  operator delete(void*);
    bool  operator ==(const trajectory&) const;

public:
    G4String              GetParticleName()    const override { return pdef_->GetParticleName(); }
    G4int                 GetPDGEncoding()     const override { return pdef_->GetPDGEncoding(); }
    G4double              GetCharge()          const override { return pdef_->GetPDGCharge(); }
    G4VTrajectoryPoint*   GetPoint(G4int i)    const override { return (*trjpoints_)[i]; }
    int                   GetPointEntries()    const override { return trjpoints_->size(); }
    G4ThreeVector         GetInitialMomentum() const override { return initial_momentum_; }
    G4int                 GetTrackID()         const override { return trackId_; }
    G4int                 GetParentID()        const override { return parentId_; }

    void ShowTrajectory (std::ostream& os) const override { G4VTrajectory::ShowTrajectory(os); }
    void DrawTrajectory ()                 const override { G4VTrajectory::DrawTrajectory(); }
    void AppendStep     (const G4Step*)          override;
    void MergeTrajectory(G4VTrajectory*)         override;

    G4ParticleDefinition* GetParticleDefinition() const { return pdef_; }
    G4double              GetInitialTime()        const { return initial_time_; }
    G4String              GetInitialVolume()      const { return initial_volume_; }
    G4ThreeVector         GetInitialPosition()    const { return initial_position_; }
    G4ThreeVector         GetFinalMomentum()      const { return final_momentum_; }
    G4ThreeVector         GetFinalPosition()      const { return final_position_; }
    G4double              GetFinalTime()          const { return final_time_; }
    G4String              GetFinalProcess()       const { return final_process_; }
    G4String              GetFinalVolume()        const { return final_volume_; }
    G4double              GetTrackLength()        const { return length_; }
    G4double              GetEnergyDeposit()      const { return edep_; }
    G4String              GetCreatorProcess()     const { return creator_process_; }


    void SetFinalMomentum(const G4ThreeVector& m) { final_momentum_ = m; }
    void SetFinalPosition(const G4ThreeVector& x) { final_position_ = x; }
    void SetFinalTime    (G4double t)             { final_time_ = t; }
    void SetTrackLength  (G4double l)             { length_ = l; }
    void SetEnergyDeposit(G4double e)             { edep_ = e; }
    void SetFinalProcess (G4String fp)            { final_process_ = fp; }
    void SetFinalVolume  (G4String fv)            { final_volume_ = fv; }



private:
    G4ParticleDefinition* pdef_;

    G4int trackId_;
    G4int parentId_;

    G4ThreeVector initial_momentum_;
    G4ThreeVector final_momentum_;

    G4ThreeVector initial_position_;
    G4ThreeVector final_position_;

    G4double initial_time_;
    G4double final_time_;

    G4double length_;
    G4double edep_;

    G4String creator_process_;
    G4String final_process_;

    G4String initial_volume_;
    G4String final_volume_;

    G4bool record_trjpoints_;

    TrajectoryPointContainer* trjpoints_;

};


#if defined G4TRACKING_ALLOC_EXPORT
extern G4DLLEXPORT G4Allocator<trajectory> TrjAllocator;
#else
extern G4DLLIMPORT G4Allocator<trajectory> TrjAllocator;
#endif

inline void* trajectory::operator new   (size_t)    { return ((void*) TrjAllocator.MallocSingle()); }
inline void  trajectory::operator delete(void* trj) { TrjAllocator.FreeSingle((trajectory*) trj); }


#endif
