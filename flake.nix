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

      # Espressif's QEMU fork (https://github.com/espressif/qemu/releases).
      qemuEspRelease = {
        tag = "esp-develop-9.2.2-20260417";
        version = "9.2.2-20260417";
        hashes = {
          "x86_64-linux" = "0eecb2a34a5586c0e59110f77b9343b7b336e82fdb0e1a30e1dc1bab8a547e35";
          "aarch64-linux" = "00de5985094c14e47d1b38464a006b8ed64fd0fa7a289c56da24f3a329f65339";
        };
        triples = {
          "x86_64-linux" = "x86_64-linux-gnu";
          "aarch64-linux" = "aarch64-linux-gnu";
        };
      };

      mkPackages =
        pkgs:
        let
          inherit (pkgs) lib;
          system = pkgs.stdenv.hostPlatform.system;

          qemu-esp32 = pkgs.stdenv.mkDerivation {
            pname = "qemu-esp-xtensa";
            inherit (qemuEspRelease) version;
            src = pkgs.fetchurl {
              url = "https://github.com/espressif/qemu/releases/download/${qemuEspRelease.tag}/qemu-xtensa-softmmu-esp_develop_${
                builtins.replaceStrings [ "-" ] [ "_" ] qemuEspRelease.version
              }-${qemuEspRelease.triples.${system}}.tar.xz";
              sha256 = qemuEspRelease.hashes.${system};
            };
            nativeBuildInputs = [ pkgs.autoPatchelfHook ];
            buildInputs = with pkgs; [
              pixman
              libgcrypt
              SDL2
              zlib
              libslirp
            ];
            dontConfigure = true;
            dontBuild = true;
            installPhase = ''
              runHook preInstall
              mkdir -p $out
              cp -r bin share $out/
              runHook postInstall
            '';
            meta = {
              description = "Espressif fork of QEMU with ESP32 (Xtensa) machine support";
              homepage = "https://github.com/espressif/qemu";
              mainProgram = "qemu-system-xtensa";
              platforms = systems;
            };
          };

          # Not packaged in nixpkgs; required by Crab_API (see Crab_API/pyproject.toml).
          python = pkgs.python3.override {
            packageOverrides = pyfinal: pyprev: {
              tomlkit-extras = pyfinal.buildPythonPackage rec {
                pname = "tomlkit_extras";
                version = "0.2.0";
                format = "wheel";
                src = pyfinal.fetchPypi {
                  inherit pname version format;
                  dist = "py3";
                  python = "py3";
                  sha256 = "03cebed47c755b1151c7a2551a74167ca34f7569058051f73c1ee0e1127e6537";
                };
                dependencies = with pyfinal; [
                  tomlkit
                  pyrsistent
                  charset-normalizer
                  pathvalidate
                  typing-extensions
                ];
                pythonImportsCheck = [ "tomlkit_extras" ];
              };
            };
          };

          crabPython = python.withPackages (
            ps: with ps; [
              pyserial
              tomlkit
              tomlkit-extras
              pytest
            ]
          );

          # Shared prologue for the helper scripts: locate the repo and firmware build dir.
          findRepo = ''
            find_repo() {
              if [ -n "''${URCHIN_ROOT:-}" ]; then echo "$URCHIN_ROOT"; return; fi
              dir="$PWD"
              while [ "$dir" != "/" ]; do
                if [ -f "$dir/Urchin_Firmware/platformio.ini" ]; then echo "$dir"; return; fi
                dir="$(dirname "$dir")"
              done
              echo "error: could not find the Urchin-Framework repo (run inside it or set URCHIN_ROOT)" >&2
              exit 1
            }
            ROOT="$(find_repo)"
            FW="$ROOT/Urchin_Firmware"
            ENV_NAME="''${URCHIN_PIO_ENV:-Urchin-Firmware}"
            BUILD="$FW/.pio/build/$ENV_NAME"
            IMAGE="''${URCHIN_FLASH_IMAGE:-$FW/urchin_flash.bin}"
            export ROOT FW ENV_NAME BUILD IMAGE
          '';

          urchin-build = pkgs.writeShellApplication {
            name = "urchin-build";
            runtimeInputs = [ pkgs.platformio ];
            text = ''
              ${findRepo}
              cd "$FW"
              exec pio run -e "$ENV_NAME" "$@"
            '';
          };

          urchin-image = pkgs.writeShellApplication {
            name = "urchin-image";
            runtimeInputs = [ pkgs.esptool ];
            text = ''
              ${findRepo}
              for f in bootloader.bin partitions.bin firmware.bin; do
                if [ ! -f "$BUILD/$f" ]; then
                  echo "error: $BUILD/$f not found; run urchin-build first" >&2
                  exit 1
                fi
              done
              esptool --chip esp32 merge-bin \
                -o "$IMAGE" --pad-to-size 4MB \
                0x1000 "$BUILD/bootloader.bin" \
                0x8000 "$BUILD/partitions.bin" \
                0x10000 "$BUILD/firmware.bin"
              echo "Wrote $IMAGE"
            '';
          };

          urchin-sim = pkgs.writeShellApplication {
            name = "urchin-sim";
            runtimeInputs = [
              urchin-build
              urchin-image
              qemu-esp32
            ];
            text = ''
              ${findRepo}
              usage() {
                cat <<EOF
              Usage: urchin-sim [--no-build] [-- extra qemu args...]

              Builds the Urchin firmware, merges it into a 4MB flash image and boots it
              in Espressif's QEMU. UART0 and UART1 are exposed as TCP servers.

                --no-build   Skip 'pio run' and reuse the existing build output
                -h, --help   Show this help

              Environment:
                URCHIN_SIM_UART0_PORT  TCP port for UART0 (default 5555)
                URCHIN_SIM_UART1_PORT  TCP port for UART1 (default 5556)
                URCHIN_PIO_ENV         PlatformIO env (default Urchin-Firmware)
                URCHIN_FLASH_IMAGE     Flash image path (default Urchin_Firmware/urchin_flash.bin)
                URCHIN_ROOT            Repo root (default: found by walking up from \$PWD)

              Point the Crab API at it with:
                URCHIN_PORT=socket://127.0.0.1:\''${URCHIN_SIM_UART0_PORT:-5555}
              Quit QEMU with Ctrl-A X.
              EOF
              }

              build=1
              while [ $# -gt 0 ]; do
                case "$1" in
                  --no-build) build=0; shift ;;
                  -h|--help) usage; exit 0 ;;
                  --) shift; break ;;
                  *) echo "unknown option: $1" >&2; usage >&2; exit 1 ;;
                esac
              done

              if [ "$build" = 1 ]; then
                urchin-build
              fi
              urchin-image

              uart0="''${URCHIN_SIM_UART0_PORT:-5555}"
              uart1="''${URCHIN_SIM_UART1_PORT:-5556}"
              echo "Starting ESP32 simulator: UART0 -> tcp://127.0.0.1:$uart0, UART1 -> tcp://127.0.0.1:$uart1 (Ctrl-A X to quit)"
              exec qemu-system-xtensa -nographic -machine esp32 \
                -drive "file=$IMAGE,if=mtd,format=raw" \
                -serial "tcp::$uart0,server=on,wait=off" \
                -serial "tcp::$uart1,server=on,wait=off" \
                "$@"
            '';
          };
        in
        {
          inherit
            qemu-esp32
            crabPython
            urchin-build
            urchin-image
            urchin-sim
            ;
        };
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
              echo "Urchin dev shell: urchin-build | urchin-image | urchin-sim  (urchin-sim --help)"
            '';
          };
        }
      );

      formatter = forAllSystems (pkgs: pkgs.nixfmt-rfc-style);
    };
}
