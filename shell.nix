{
  pkgs ? import nix/pkgs.nix
}:

let
  derivation = pkgs.callPackage (import ./nix/derivation.nix) {};
in

pkgs.llvmPackages_11.stdenv.mkDerivation {
  inherit (derivation) name nativeBuildInputs;

  buildInputs = derivation.buildInputs ++ [
    pkgs.clang-tools
    pkgs.clang_11
    pkgs.bear
    pkgs.cmake
  ];

}
