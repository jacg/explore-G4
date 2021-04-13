#ifndef HIT_H
#define HIT_H

#include <G4VHit.hh>
#include <G4THitsCollection.hh>
#include <G4Allocator.hh>
#include <G4ThreeVector.hh>


class sensor_hit: public G4VHit {
public:
    sensor_hit(const G4ThreeVector& position);
    sensor_hit(const sensor_hit&);
    ~sensor_hit() {}

    const sensor_hit& operator=(const sensor_hit&);
    G4int operator==(const sensor_hit& other) const { return (this==&other) ? 1 : 0; }

    void* operator new   (size_t);
    void  operator delete(void* sensor_hit);

    G4ThreeVector get_position() const { return position_; }
    void set_position(const G4ThreeVector& position) { position_ = position ; }

private:
    G4ThreeVector position_;
};

typedef G4THitsCollection<sensor_hit> sensor_hits_collection;
extern G4Allocator<sensor_hit> sensor_hit_allocator;

inline void* sensor_hit::operator new(size_t) { return ((void*) sensor_hit_allocator.MallocSingle()); }
inline void  sensor_hit::operator delete(void* hit) { sensor_hit_allocator.FreeSingle((sensor_hit*) hit); }

#endif
