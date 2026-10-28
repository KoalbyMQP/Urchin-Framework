{
  description = "Urchin Framework dev environment: Crab API (Python), Urchin Firmware (ESP-IDF via PlatformIO) and an ESP32 QEMU simulator";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});

      # Package definitions live in ./nix.
      mkPackages = import ./nix;
    in
    {
      packages = forAllSystems (
        pkgs:
        let
          p = mkPackages pkgs;
        in
        {
          inherit (p)
            qemu-esp32
            urchin-build
            urchin-image
            urchin-motor-sim
            urchin-sim
            ;
          default = p.urchin-sim;
        }
      );

      apps = forAllSystems (
        pkgs:
        let
          p = self.packages.${pkgs.stdenv.hostPlatform.system};
          app = drv: {
            type = "app";
            program = pkgs.lib.getExe drv;
          };
        in
        {
          sim = app p.urchin-sim;
          motor-sim = app p.urchin-motor-sim;
          build = app p.urchin-build;
          image = app p.urchin-image;
          default = app p.urchin-sim;
        }
      );

      devShells = forAllSystems (
        pkgs:
        let
          p = mkPackages pkgs;
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              # Crab API
              p.crabPython
              ruff
              uv
              pre-commit

              # Urchin Firmware. PlatformIO (FHS-wrapped so its downloaded
              # ESP-IDF toolchains run on NixOS too) fetches the Xtensa GCC,
              # ESP-IDF 5.x, cmake and ninja into ~/.platformio on first build.
              platformio
              esptool
              clang-tools
              cmake
              ninja
              doxygen

              # ESP32 simulator + helpers
              p.qemu-esp32
              p.urchin-build
              p.urchin-image
              p.urchin-motor-sim
              p.urchin-sim
              picocom
              socat
              netcat-openbsd
            ];

            shellHook = ''
              # Crab API modules use flat imports (e.g. `from ESPSerial import *`).
              URCHIN_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || echo "$PWD")"
              export URCHIN_ROOT
              export PYTHONPATH="$URCHIN_ROOT/Crab_API/src''${PYTHONPATH:+:$PYTHONPATH}"
              echo "Urchin dev shell: urchin-build | urchin-image | urchin-sim | urchin-motor-sim  (--help on each)"
            '';
          };
        }
      );

      formatter = forAllSystems (pkgs: pkgs.nixfmt-rfc-style);
    };
}
