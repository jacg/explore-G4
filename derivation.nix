{ boost, poco, stdenv }:

stdenv.mkDerivation {
  name = "explore-gee-fore";
  src = ./.;

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
