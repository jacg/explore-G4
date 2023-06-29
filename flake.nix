# The core functionality is provided here using flakes. Legacy support for
# `nix-shell` is provided by a wrapper in `shell.nix`.

{
  description = "Geant4 development environment";

  inputs = {

    # Version pinning is managed in flake.lock. Upgrading can be done with
    # something like
    #
    #    nix flake lock --update-input nixpkgs

    newpkgs         .url = "github:nixos/nixpkgs/nixos-23.05";
    nixpkgs         .url = "github:nixos/nixpkgs/nixos-21.11";
    flake-utils     .url = "github:numtide/flake-utils";
    flake-compat = { url = "github:edolstra/flake-compat"; flake = false; };
   #nixgl           .url = "github:guibou/nixGL";
  };

  outputs = { self, nixpkgs, newpkgs, flake-utils, ... }:

    # Option 1: try to support each default system
    flake-utils.lib.eachDefaultSystem # NB Some packages in nixpkgs are not supported on some systems

    # Option 2: try to support selected systems
    # flake-utils.lib.eachSystem ["x86_64-linux" "i686-linux" "aarch64-linux" "x86_64-darwin"]
      (system:

        let pkgs = import nixpkgs {
              inherit system;
              overlays = [
                #nixgl.overlay
              ];
            };

            # ----- Conditional inclusion ----------------------------------------------------
            nothing = pkgs.coreutils;
            linux   = drvn: if pkgs.stdenv.isLinux  then drvn else nothing;

            # ----- Getting OpenGL to work on non-NixOS --------------------------------------
            nixGL = pkgs.callPackage "${builtins.fetchTarball {
              url = https://github.com/guibou/nixGL/archive/7d6bc1b21616bab6cf4a6520c2639a11c25a220e.tar.gz;
              #sha256 = pkgs.lib.fakeSha256;
              sha256 = "02y38zmdplk7a9ihsxvnrzhhv7324mmf5g8hmxqizaid5k5ydpr3";
            }}/nixGL.nix" {};

            new = import newpkgs { inherit system; };
        in
          rec {

            python-version = "39";
            python-N = "python${python-version}";

            #devShell = pkgs.mkShell.override { stdenv = pkgs.clang13Stdenv; } {
            devShell = pkgs.mkShell.override { stdenv = pkgs.clang_13.stdenv; } {
            #devShell = pkgs.llvmPackages_13.stdenv.mkDerivation {
              name = "Geant4-development-environment";

              packages = [
                pkgs.clang_13
                pkgs.cmake-language-server
                #pkgs.clang-tools


                (pkgs.geant4.override {
                  enableMultiThreading = false;
                  enableInventor       = false;
                  enableQT             = true;
                  enableXM             = false;
                  enableOpenGLX11      = true;
                  enablePython         = false;
                  enableRaytracerX11   = false;
                })

                pkgs.geant4.data.G4PhotonEvaporation
                pkgs.geant4.data.G4EMLOW
                pkgs.geant4.data.G4RadioactiveDecay
                pkgs.geant4.data.G4ENSDFSTATE
                pkgs.geant4.data.G4SAIDDATA
                pkgs.geant4.data.G4PARTICLEXS
                pkgs.geant4.data.G4NDL

                (pkgs.${ python-N }.withPackages (ps: [ps.docopt]))
                new.cmake
                pkgs.catch2
                pkgs.gdb
                pkgs.hdf5
                pkgs.highfive
                pkgs.hdfview
                pkgs.just
                # For big-endian writing of density image in raw format
                pkgs.poco
                pkgs.pcre # Needed by Poco
                # For graphics hardware matching on non-NixOS
                #                pkgs.libGL
                #                (linux nixGL.nixGLDefault)
                # Syntax colouring in gdb
                pkgs.${ python-N }.pkgs.pygments
                # profiling
                (linux pkgs.linuxPackages.perf)
                (linux pkgs.oprofile)
                (linux pkgs.kcachegrind)
                (linux pkgs.graphviz) # used by kcachegrind
                (linux pkgs.flamegraph)
              ];

              QT_QPA_PLATFORM_PLUGIN_PATH="${pkgs.libsForQt5.qt5.qtbase.bin}/lib/qt-${pkgs.libsForQt5.qt5.qtbase.version}/plugins";
            };
          }
      );
}
