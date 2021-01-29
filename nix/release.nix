# Build project with multiple combinations of compilers and dependencies
#
# Usage: in project top-level:
#
#    nix-build nix/release.nix

{
  pkgs ? import ./pkgs.nix
}:

let
  compilers = with pkgs; {
    gcc7    = overrideCC stdenv gcc7;
    gcc8    = overrideCC stdenv gcc8;
    gcc9    = overrideCC stdenv gcc9;
    gcc10   = overrideCC stdenv gcc10;
    clang9  = overrideCC stdenv clang_9;
    clang10 = overrideCC stdenv clang_10;
    clang11 = overrideCC stdenv clang_11;
  };

  pocoLibs = {
    poco10 = pkgs.poco;
    poco9 = pkgs.poco.overrideAttrs (oldAttrs: {
      name = "poco-1.9.1";
      src = pkgs.fetchgit {
        url = "https://github.com/pocoproject/poco.git";
        rev = "196540ce34bf884921ff3f9ce338e38fc938acdd";
        sha256 = "0q0xihkm2z8kndx40150inq7llcyny59cv016gxsx0vbzzbdkcnd";
      };
    });
  };

  boostLibs = {
    inherit (pkgs) boost173 boost174 boost175;
  };

  myProject = import ./derivation.nix;

in
 [
   (myProject { stdenv = compilers.gcc10;   geant4 = pkgs.geant4; })
   (myProject { stdenv = compilers.clang11; geant4 = pkgs.geant4; })
 ]
