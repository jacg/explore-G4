# To update `commit-id` go to https://status.nixos.org/, which lists the
# latest commit that passes all the tests for any release. Unless there is an
# overriding reason, pick the latest stable NixOS release, at the time of
# writing this is nixos-20.09.
let
  name = "nixos-unstable-2021-01-28";
  commit-id = "4c9a74aa459dc525fcfdfb3019b234f68de66c8a";
  url = "https://github.com/nixos/nixpkgs/archive/${commit-id}.tar.gz";

  geant4-debug-overlay = final: previous: {
   geant4 = previous.geant4.overrideAttrs (old: {
     cmakeBuildType = "Debug";
   });

};

in
  import (builtins.fetchTarball { url = url; })
    {
      # overlays = [ geant4-debug-overlay ];
    }
