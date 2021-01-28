{
  pkgs ? import nix/pkgs.nix
}:

let
  derivation = pkgs.callPackage (import ./nix/derivation.nix) {};
in

pkgs.mkShell {
  inherit (derivation) name nativeBuildInputs;

  buildInputs = derivation.buildInputs ++ [
    pkgs.clang_11
    pkgs.bear
  ];

}
