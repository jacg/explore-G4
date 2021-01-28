{
  pkgs ? import nix/pkgs.nix
}:

let
  buildIt = import nix/derivation.nix;
in

pkgs.callPackage buildIt { inherit (pkgs) boost stdenv; }
