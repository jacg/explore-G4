#include <geometries/inspect.hh>

#include <G4TransportationManager.hh>

#include <Poco/ByteOrder.h>
#include <Poco/BinaryWriter.h>

#include <iostream>
#include <fstream>

#include <chrono>

world_geometry_inspector::world_geometry_inspector(G4RunManager* run_manager)
  : navigator{std::make_unique<G4Navigator>()}
{
  run_manager -> Initialize(); // ensure that geometry is closed
  G4VPhysicalVolume* world = G4TransportationManager::GetTransportationManager()
    -> GetNavigatorForTracking()
    -> GetWorldVolume();
  navigator -> SetWorldVolume(world);
}


G4Material const * world_geometry_inspector::material_at(const G4ThreeVector& point) const {
  return volume_at(point) -> GetLogicalVolume() -> GetMaterial();
}

G4VPhysicalVolume const* world_geometry_inspector::volume_at(const G4ThreeVector& point) const {
  auto touchable = std::make_unique<G4TouchableHistory>();
  navigator -> LocateGlobalPointAndUpdateTouchable(point, touchable.get(), false);
  return touchable -> GetVolume();
}

void attenuation_map(std::tuple<float, float, float> fov_full_size,
                     std::tuple<unsigned short, unsigned short, unsigned short> n_voxels,
                     std::string filename,
                     world_geometry_inspector& inspect) {
  auto [DX, DY, DZ] = fov_full_size;
  auto [nx, ny, nz] = n_voxels;
  auto dx = DX/nx;
  auto dy = DY/ny;
  auto dz = DZ/nz;

  std::ofstream out{filename, std::ios::out | std::ios::binary};
  if (!out.good()) {
    throw "Failed to open attenuation image file: " + filename;
  }
  std::cout << "Writing attenuation map to: " << filename << std::endl;

  Poco::BinaryWriter write{out, Poco::BinaryWriter::BIG_ENDIAN_BYTE_ORDER};
  write << nx << ny << nz << DX << DY << DZ;

  std::vector<float> image;
  image.reserve(nx * ny * nz);

  auto start = std::chrono::steady_clock::now();
  for (auto iz=0; iz<nz; ++iz) {
    std::cout << "iz = " << iz << std::endl;
    for (auto iy=0; iy<ny; ++iy) {
      for (auto ix=0; ix<nx; ++ix) {
        auto x = (dx-DX) / 2 + ix * dx;
        auto y = (dy-DY) / 2 + iy * dy;
        auto z = (dz-DZ) / 2 + iz * dz;
        G4ThreeVector point{x, y, z};
        auto density = inspect.material_at(point) -> GetDensity();
        // auto material = inspect.material_at(point);
        // auto density = material -> GetDensity();
        // auto matname = material -> GetName();
        // auto name = inspect.volume_at(point) -> GetName();
        // using std::setw; using std::cout; using std::endl;
        // using CLHEP::kg; using CLHEP::m3;
        // cout << setw(6) << x
        //      << setw(6) << y
        //      << setw(6) << z
        //      << " Density: "  << setw(10) << density / (kg / m3)
        //      << setw(15) << matname
        //      << "   " << name << endl;

        //image.push_back(density);
        write << static_cast<float>(density);
      }
    }
  }
  //write << image;
  auto stop = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_seconds = stop - start;
  std::cout << "Done" << std::endl;
  auto seconds = elapsed_seconds.count();
  std::cout << "Took " << seconds << " (" << (nx * ny * nz) / seconds << " pixels per second)\n";
}
