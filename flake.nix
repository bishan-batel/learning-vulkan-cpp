##
## Sep 5. 2025 
## Kishan S Patel
##

{
  description = "A Nix-flake-based C/C++ development environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { 
    self, nixpkgs, flake-utils, ...
    }: flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      llvm = pkgs.llvmPackages_19;
      apple = pkgs.apple-sdk;
      libPath = with pkgs; [ 
        glm 
        glfw
        shaderc
        vulkan-headers
        vulkan-loader
        vulkan-tools
        vulkan-validation-layers
      ] ++ (if pkgs.stdenv.isDarwin then [
          # Darwin Specific
          apple
          pkgs.moltenvk
        ] else with pkgs; [ 
            # Linux Specific
            alsa-lib 
            wayland-protocols
            wayland
            libxkbcommon
            libffi
            xorg.libX11.dev
            xorg.libxcb
            xorg.libX11.dev
            xorg.libXft
            xorg.libXrandr
            xorg.libXinerama
            xorg.libXcursor
            xorg.libXi
            glfw.dev
            extra-cmake-modules
            libxkbcommon
          ]);
    in {

      devShells = { 
        default = pkgs.mkShell.override {
          stdenv = llvm.stdenv;
        } {
            name = "na-engine";
            packages = with pkgs; [
              ninja
              cmake
              extra-cmake-modules
              clang-tools
              lldb
            ];

            buildInputs = with pkgs; [
              # gcc
              libffi
              llvm.lldb 
              llvm.clang-tools
              llvm.clang

              pkg-config
            ];
            nativeBuildInputs = libPath;

            LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath libPath;
            VULKAN_SDK = "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";
          };
      };
    });
}
