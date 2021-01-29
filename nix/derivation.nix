# Fundamental derivation of how to build the project: a function from
# dependencies to build-result. Picking the specific versions of the
# dependencies is Someone Else's Problem: `default.nix`, `release.nix` get to
# pick these.

{ geant4, stdenv }:

stdenv.mkDerivation {
  name = "explore-gee-fore";
  src = ../.;

  # build-time dependencies
  nativeBuildInputs = [
    geant4
    geant4.data.G4PhotonEvaporation
    geant4.data.G4EMLOW
    geant4.data.G4RadioactiveDecay
    geant4.data.G4ENSDFSTATE
    geant4.data.G4SAIDDATA
    geant4.data.G4PARTICLEXS
    geant4.data.G4NDL

  ];

  # run-time dependencies
  buildInputs = [

  ];

  buildPhase = ''
    c++ -std=c++17 -o main main.cc
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp main $out/bin/
  '';

}
