{
  pkgs ? import nix/pkgs.nix
}:

pkgs.callPackage (import nix/derivation.nix) { }
