{
  description = "A Nix-flake-based C/C++ development environment";

  inputs = {
    nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";
    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
  };

  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = [
        "x86_64-linux"
        "aarch64-darwin"
      ];

      perSystem = {pkgs, ...}: {
        formatter = pkgs.alejandra;

        devShells.default =
          pkgs.mkShell.override
          {stdenv = pkgs.llvmPackages_13.stdenv;}
          {
            packages = with pkgs;
              [
                cmake
                z3
                gperftools
                sqlite
                ninja
                cppcheck
                lit
              ]
              ++ (with pkgs.llvmPackages_13; [
                libllvm
                libcxx
                clang-tools
              ]);
          };
      };
    };
}
