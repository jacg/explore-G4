{
  pkgs ? import nix/pkgs.nix
}:

let
  derivation = pkgs.callPackage (import ./nix/derivation.nix) {};

# ----- Conditional inclusion ----------------------------------------------------
  nothing = pkgs.coreutils;
  linux      = drvn: if pkgs.stdenv.isLinux  then drvn else nothing;

  # ----- Getting OpenGL to work on non-NixOS --------------------------------------
  nixGL = pkgs.callPackage "${builtins.fetchTarball {
    url = https://github.com/guibou/nixGL/archive/7d6bc1b21316bab6cf4a6520c2639a11c25a220e.tar.gz;
    #sha256 = pkgs.lib.fakeSha256;
    sha256 = "02y38zmdplk7a9ihsxvnrzhhv7324mmf5g8hmxqizaid5k5ydpr3";
  }}/nixGL.nix" {};
in

pkgs.llvmPackages_11.stdenv.mkDerivation {
  inherit (derivation) name;
  nativeBuildInputs = derivation.nativeBuildInputs ++ [
    pkgs.clang_11
  ];

  buildInputs = derivation.buildInputs ++ [
#    pkgs.clang-tools
    pkgs.clang_11
    pkgs.cmake
    pkgs.catch2
    pkgs.cmake-language-server
    pkgs.gdb
    pkgs.hdf5
    pkgs.highfive
    pkgs.hdfview
    # For graphics hardware matching on non-NixOS
    pkgs.libGL
    (linux nixGL.nixGLDefault)
    # Syntax colouring in gdb
    pkgs.python3Packages.pygments
    # profiling
    (linux pkgs.linuxPackages.perf)
    (linux pkgs.oprofile)
    (linux pkgs.kcachegrind)
    (linux pkgs.graphviz) # used by kcachegrind
    (linux pkgs.flamegraph)
  ];

}
