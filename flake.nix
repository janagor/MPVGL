{
  description = "Programming Environment for C++/Vulkan/Wayland";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
      in
      {
        devShells.default = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            clang-tools
          ];

          buildInputs = with pkgs; [
            # X11
            libXi
            libX11
            libXext
            libXrandr
            libXcursor
            libXinerama

            # Wayland & Common
            libffi
            wayland
            libxkbcommon
            wayland-scanner
            wayland-protocols

            # Vulkan
            vulkan-loader
            vulkan-headers
            vulkan-validation-layers
          ];

          shellHook = ''
            export LD_LIBRARY_PATH="${
              pkgs.lib.makeLibraryPath [
                pkgs.libX11
                pkgs.vulkan-loader
                pkgs.libxkbcommon
                pkgs.wayland
              ]
            }:$LD_LIBRARY_PATH"

            echo "Developing Environment is ready!"
          '';
        };
      }
    );
}
