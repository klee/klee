{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";

  outputs = inputs:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: inputs.nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import inputs.nixpkgs { inherit system; };
      });
    in
    {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell.override
          {
            stdenv = pkgs.llvmPackages_13.stdenv;
          }
          {
            packages = with pkgs; [
              clang-tools
              cmake
              z3
              ninja
              cppcheck
              doxygen
              gtest
              lcov
            ] ++ (with pkgs.llvmPackages_13; [libllvm libcxx]);
          };
      });
    };
}
