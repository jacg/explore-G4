# TLDR

```shell
cd abracadabra/build
cmake ..
bear -- make -j8
make test
./abracadabra
```

# Caveats

## Dependency management with Nix

The above instructions assume that you have the require dependencies installed
and configured.

This is done automatically when you `cd` into the project's directory, if you
have installed the Nix package manager and `direnv`.

## Clang developer tools

Clang developer tools should largely work out of the box in the Nix-provided
environment. Crucially, they require some knowledge about how the project is
combined. This information is collected by `bear`, during compilation.
Consequently `bear` should only be used when *the whole project* is being
compiled. That is:

1. The very first time you compile the project.

2. After adding new files to the project. In this case, running `bear -- make`
   will cause `bear` to collect information about the compilation of the new
   files only, forgetting all the others. This means that the *whole* project
   must be recompiled. So you must `make clean` before running `bear -- make`.

On most other occasions, `bear` should **not** be used when running `make`.
