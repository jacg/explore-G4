# The pinned nixpkgs set used by `default.nix` and `release.nix` in the
# top-level of the project.

import (builtins.fetchGit {
    name = "nixos-unstable-2021-01-23";
    url = "https://github.com/nixos/nixpkgs-channels/";
    ref = "refs/heads/nixos-unstable";

    # To get the current value of `rev`:
    # `git ls-remote https://github.com/nixos/nixpkgs-channels nixos-unstable`
    # or check https://status.nixos.org/ to see latest versions without build problems
    rev = "4762fba469e2baa82f983b262e2c06ac2fdaae67";
  }) {}
