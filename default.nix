{
  pkgs ? import (builtins.fetchGit {
    name = "nixos-unstable-2021-01-23";
    url = "https://github.com/nixos/nixpkgs-channels/";
    ref = "refs/heads/nixos-unstable";

    # To get the current value of `rev`:
    # `git ls-remote https://github.com/nixos/nixpkgs-channels nixos-unstable`
    # or check https://status.nixos.org/ to see latest versions without build problems
    rev = "4762fba469e2baa82f983b262e2c06ac2fdaae67";
  }) {}
}:

pkgs.stdenv.mkDerivation {
  name = "explore-gee-fore";
  src = ./.;

  # build-time dependencies
  nativeBuildInputs = [
    pkgs.boost
    pkgs.poco
  ];

  # run-time dependencies
  buildInputs = [
    #pkgs.clangd
  ];

  buildPhase = ''
    c++ -std=c++17 -o main main.cc -lPocoFoundation -lboost_system
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp main $out/bin/
  '';

}
