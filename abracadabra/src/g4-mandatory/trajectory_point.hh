#ifndef TRAJECTORY_POINT_H
#define TRAJECTORY_POINT_H

#include <G4VTrajectoryPoint.hh>
#include <G4Allocator.hh>


class trajectory_point: public G4VTrajectoryPoint {
public:
    trajectory_point();
    trajectory_point(G4ThreeVector, G4double);
    trajectory_point(const trajectory_point&);
    virtual ~trajectory_point() {}

    const trajectory_point& operator =(const trajectory_point&);
    int operator ==(const trajectory_point&) const;
    void* operator new(size_t);
    void operator delete(void*);

    const G4ThreeVector GetPosition() const;
    G4double GetTime() const;

private:
    G4ThreeVector position_;
    G4double time_;
};

#if defined G4TRACKING_ALLOC_EXPORT
extern G4DLLEXPORT G4Allocator<trajectory_point> TrjPointAllocator;
#else
extern G4DLLIMPORT G4Allocator<trajectory_point> TrjPointAllocator;
#endif


inline int trajectory_point::operator ==(const trajectory_point& other) const
{return (this==&other); }

inline void* trajectory_point::operator new(size_t)
{ return ((void*) TrjPointAllocator.MallocSingle()); }

inline void trajectory_point::operator delete(void* tp)
{ TrjPointAllocator.FreeSingle((trajectory_point*) tp); }

inline const G4ThreeVector trajectory_point::GetPosition() const { return position_; }

inline G4double trajectory_point::GetTime() const { return time_; }

#endif
