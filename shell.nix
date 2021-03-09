{
  pkgs ? import nix/pkgs.nix
}:

let
  derivation = pkgs.callPackage (import ./nix/derivation.nix) {};
in

pkgs.llvmPackages_11.stdenv.mkDerivation {
  inherit (derivation) name;
  nativeBuildInputs = derivation.nativeBuildInputs ++ [
    pkgs.clang_11
    pkgs.bear
  ];

  buildInputs = derivation.buildInputs ++ [
#    pkgs.clang-tools
    pkgs.clang_11
    pkgs.cmake
    pkgs.catch2
    pkgs.cmake-language-server
    pkgs.gdb
  ];

}
