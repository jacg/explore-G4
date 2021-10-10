#include "io/raw_image.hh"

#include "nain4.hh"

#include <Poco/BinaryWriter.h>
#include <Poco/BinaryReader.h>
#include <Poco/ByteOrder.h>

raw_image::raw_image(std::string filename) {
  std::ifstream in{filename, std::ios::in | std::ios::binary};
  if (!in.good()) { FATAL(("Failed to open raw image input file: " + filename).c_str()); }
  Poco::BinaryReader read{in, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER};
  u nx, ny, nz;    read >> nx >> ny >> nz;    n = {nx, ny, nz};
  f dx, dy, dz;    read >> dx >> dy >> dz;    d = {dx, dy, dz};
  // TODO replace loop with some buffered version
  auto size = static_cast<unsigned>(nx) * ny * nz;
  pixels.reserve(size);
  for (unsigned i=0; i<size; ++i) {
    f p;
    read >> p;
    pixels.push_back(p);
  }
}

void raw_image::write(std::string filename) {
  std::ofstream out{filename, std::ios::out | std::ios::binary};
  if (!out.good()) { FATAL(("Failed to open raw image output file: " + filename).c_str()); }
  Poco::BinaryWriter write{out, Poco::BinaryWriter::BIG_ENDIAN_BYTE_ORDER};
  const auto [nx, ny, nz] = n;    write << nx << ny << nz;
  const auto [dx, dy, dz] = d;    write << dx << dy << dz;
  // TODO replace loop with some buffered version
  for (const auto p : pixels) { write << p; }
}
