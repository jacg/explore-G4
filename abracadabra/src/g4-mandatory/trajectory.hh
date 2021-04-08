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
    int   operator ==(const trajectory&) const;

public:
    G4ParticleDefinition* GetParticleDefinition();
    G4String              GetParticleName()    const;
    G4double              GetCharge()          const;
    G4int                 GetPDGEncoding ()    const;
    G4String              GetCreatorProcess()  const;
    G4int                 GetTrackID()         const;
    G4int                 GetParentID()        const;
    G4ThreeVector         GetInitialMomentum() const;
    G4ThreeVector         GetInitialPosition() const;
    G4double              GetInitialTime()     const;

    G4ThreeVector GetFinalMomentum() const;
    G4ThreeVector GetFinalPosition() const;
    G4double      GetFinalTime()     const;
    G4double      GetTrackLength()   const;
    G4double      GetEnergyDeposit() const;
    G4String      GetInitialVolume() const;
    G4String      GetFinalVolume()   const;
    G4String      GetFinalProcess()  const;

    void SetFinalMomentum(const G4ThreeVector&);
    void SetFinalPosition(const G4ThreeVector&);
    void SetFinalTime    (G4double);
    void SetTrackLength  (G4double);
    void SetEnergyDeposit(G4double);
    void SetFinalVolume  (G4String);
    void SetFinalProcess (G4String);

    virtual G4VTrajectoryPoint* GetPoint(G4int i) const;
    virtual int  GetPointEntries() const;
    virtual void AppendStep(const G4Step*);
    virtual void MergeTrajectory(G4VTrajectory*);
    virtual void ShowTrajectory(std::ostream&) const;
    virtual void DrawTrajectory() const;

private:
    /// The default constructor is private. A trajectory can
    /// only be constructed associated to a track.
    trajectory();

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

inline void trajectory::ShowTrajectory(std::ostream& os) const { G4VTrajectory::ShowTrajectory(os); }
inline void trajectory::DrawTrajectory()                 const { G4VTrajectory::DrawTrajectory(); }

inline G4ParticleDefinition* trajectory::GetParticleDefinition()    { return pdef_; }
inline G4VTrajectoryPoint*   trajectory::GetPoint(G4int i)    const { return (*trjpoints_)[i]; }
inline int                   trajectory::GetPointEntries()    const { return trjpoints_->size(); }
inline G4ThreeVector         trajectory::GetInitialMomentum() const { return initial_momentum_; }
inline G4int                 trajectory::GetTrackID()         const { return trackId_; }
inline G4int                 trajectory::GetParentID()        const { return parentId_; }
inline G4ThreeVector         trajectory::GetFinalMomentum()   const { return final_momentum_; }
inline G4ThreeVector         trajectory::GetInitialPosition() const { return initial_position_; }
inline G4ThreeVector         trajectory::GetFinalPosition()   const { return final_position_; }
inline G4double              trajectory::GetInitialTime()     const { return initial_time_; }
inline G4double              trajectory::GetFinalTime()       const { return final_time_; }
inline G4double              trajectory::GetTrackLength()     const { return length_; }
inline G4double              trajectory::GetEnergyDeposit()   const { return edep_; }
inline G4String              trajectory::GetCreatorProcess()  const { return creator_process_; }
inline G4String              trajectory::GetFinalProcess()    const { return final_process_; }
inline G4String              trajectory::GetInitialVolume()   const { return initial_volume_; }
inline G4String              trajectory::GetFinalVolume()     const { return final_volume_; }
inline G4String              trajectory::GetParticleName()    const { return pdef_->GetParticleName(); }
inline G4int                 trajectory::GetPDGEncoding()     const { return pdef_->GetPDGEncoding(); }
inline G4double              trajectory::GetCharge()          const { return pdef_->GetPDGCharge(); }

inline void trajectory::SetFinalMomentum(const G4ThreeVector& m) { final_momentum_ = m; }
inline void trajectory::SetFinalPosition(const G4ThreeVector& x) { final_position_ = x; }
inline void trajectory::SetFinalTime    (G4double t)             { final_time_ = t; }
inline void trajectory::SetTrackLength  (G4double l)             { length_ = l; }
inline void trajectory::SetEnergyDeposit(G4double e)             { edep_ = e; }
inline void trajectory::SetFinalProcess (G4String fp)            { final_process_ = fp; }
inline void trajectory::SetFinalVolume  (G4String fv)            { final_volume_ = fv; }

#endif
