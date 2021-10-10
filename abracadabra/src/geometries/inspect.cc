#include <geometries/inspect.hh>
#include "io/raw_image.hh"

#include <G4SystemOfUnits.hh>
#include <G4TransportationManager.hh>
#include <G4UnitsTable.hh>

#include <iostream>
#include <fstream>

#include <chrono>

using WGI = world_geometry_inspector;
using f   = float;
using u   = unsigned short;

WGI::world_geometry_inspector(G4RunManager* run_manager)
  : navigator{std::make_unique<G4Navigator>()}
  , touchable{std::make_unique<G4TouchableHistory>()}
{
  run_manager -> Initialize(); // ensure that geometry is closed
  auto world = G4TransportationManager::GetTransportationManager()
    -> GetNavigatorForTracking()
    -> GetWorldVolume();
  navigator -> SetWorldVolume(world);
}

G4Material const * WGI::material_at(const G4ThreeVector& point) const {
  return volume_at(point) -> GetLogicalVolume() -> GetMaterial();
}

G4VPhysicalVolume const* WGI::volume_at(const G4ThreeVector& point) const {
  navigator -> LocateGlobalPointAndUpdateTouchable(point, touchable.get(), false);
  return touchable -> GetVolume();
}

void WGI::attenuation_map(std::tuple<f,f,f> fov_full_size, std::tuple<u,u,u> n_voxels, std::string filename) {
  auto [DX, DY, DZ] = fov_full_size;
  auto [nx, ny, nz] = n_voxels;
  auto dx = DX/nx;
  auto dy = DY/ny;
  auto dz = DZ/nz;

  std::ofstream out{filename, std::ios::out | std::ios::binary};
  if (!out.good()) {
    throw "Failed to open attenuation image file: " + filename;
  }
  std::cout
    << "Calculating attenuation map with "
    << nx << " x " << ny << " x " << nz << " voxels across "
    << DX << " x " << DY << " x " << DZ << " mm" << std::endl;

  std::vector<float> pixels;
  pixels.reserve(nx * ny * nz);

  auto start = std::chrono::steady_clock::now();
  for (auto iz=0; iz<nz; ++iz) {
    for (auto iy=0; iy<ny; ++iy) {
      for (auto ix=0; ix<nx; ++ix) {
        auto x = (dx-DX) / 2 + ix * dx;
        auto y = (dy-DY) / 2 + iy * dy;
        auto z = (dz-DZ) / 2 + iz * dz;
        auto density = material_at({x,y,z}) -> GetDensity() / (kg/m3);
        pixels.push_back(static_cast<float>(density));
      }
    }
  }
  auto stop = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_seconds = stop - start;
  std::cout << "Done" << std::endl;
  auto seconds = elapsed_seconds.count();
  auto pps = (nx * ny * nz) / (seconds * s);
  std::cout << "Took " << seconds << " seconds"
            << " (Time per pixel: " << G4BestUnit(1/pps, "Time")
            << ", rate: " << G4BestUnit(pps, "Frequency") << ")\n";

  raw_image{n_voxels, fov_full_size, std::move(pixels)}.write(filename);
  std::cout << "Wrote attenuation map to: " << filename << std::endl;
}
