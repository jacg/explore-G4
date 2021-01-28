# To update `commit-id` go to https://status.nixos.org/, which lists the
# latest commit that passes all the tests for any release. Unless there is an
# overriding reason, pick the latest stable NixOS release, at the time of
# writing this is nixos-20.09.
let
  name = "nixos-unstable-2021-01-28";
  commit-id = "15a64b2facc1b91f4361bdd101576e8886ef834b";
  url = "https://github.com/nixos/nixpkgs/archive/${commit-id}.tar.gz";
in
  import (builtins.fetchTarball { url = url; }) {}
