#ifndef TRAJECTORY_MAP_H
#define TRAJECTORY_MAP_H

#include <map>

class G4VTrajectory;

class trajectory_map {
public:
	static G4VTrajectory* Get(int trackId);
	static void Add(G4VTrajectory*);
	static void Clear();

private:
	// Constructors, destructor and assignement op are hidden
	// so that no instance of the class can be created.
	trajectory_map();
	trajectory_map(const trajectory_map&);
	~trajectory_map();

private:
	static std::map<int, G4VTrajectory*> map_;
};


inline trajectory_map::trajectory_map() {}
inline trajectory_map::~trajectory_map() { map_.clear(); }
inline void trajectory_map::Clear() { map_.clear(); }

#endif
