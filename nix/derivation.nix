# Fundamental derivation of how to build the project: a function from
# dependencies to build-result. Picking the specific versions of the
# dependencies is Someone Else's Problem: `default.nix`, `release.nix` get to
# pick these.

{ boost, poco, stdenv }:

stdenv.mkDerivation {
  name = "explore-gee-fore";
  src = ../.;

  # build-time dependencies
  nativeBuildInputs = [
    boost
    poco
  ];

  # run-time dependencies
  buildInputs = [
    #clangd
  ];

  buildPhase = ''
    c++ -std=c++17 -o main main.cc -lPocoFoundation -lboost_system
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp main $out/bin/
  '';

}
