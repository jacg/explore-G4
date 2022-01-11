# To update `commit-id` go to https://status.nixos.org/, which lists the
# latest commit that passes all the tests for any release. Unless there is an
# overriding reason, pick the latest stable NixOS release, at the time of
# writing this is nixos-20.09.
let
  name = "nixos-21.11-2022-01-11";
  commit-id = "386234e2a61e1e8acf94dfa3a3d3ca19a6776efb";
  url = "https://github.com/nixos/nixpkgs/archive/${commit-id}.tar.gz";

  geant4-debug-overlay = final: previous: {
   geant4 = previous.geant4.overrideAttrs (old: {
     cmakeBuildType = "Debug";
     dontStrip = true;
   });

};

in
  import (builtins.fetchTarball { url = url; })
    {
      # overlays = [ geant4-debug-overlay ];
    }
