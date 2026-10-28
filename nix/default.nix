# All packages provided by the flake, for one nixpkgs instance.
pkgs:
let
  # Every helper script starts with find-repo.sh, which locates the repo and
  # the firmware build output.
  script =
    name: runtimeInputs:
    pkgs.writeShellApplication {
      inherit name runtimeInputs;
      text = builtins.readFile ./scripts/find-repo.sh + builtins.readFile ./scripts/${name}.sh;
    };
in
rec {
  qemu-esp32 = pkgs.callPackage ./qemu-esp32.nix { };

  crabPython = pkgs.callPackage ./python.nix { };

  # HerkuleX servo emulator for the simulator's motor bus (UART1). Only the
  # standard library is needed; -I keeps a caller's PYTHONPATH (e.g. a ROS
  # environment) from leaking in.
  urchin-motor-sim = pkgs.writeShellApplication {
    name = "urchin-motor-sim";
    runtimeInputs = [ pkgs.python3 ];
    text = ''exec python3 -I ${./motor-sim/urchin_motor_sim.py} "$@"'';
  };

  urchin-build = script "urchin-build" [ pkgs.platformio ];

  urchin-image = script "urchin-image" [ pkgs.esptool ];

  urchin-sim = script "urchin-sim" [
    urchin-build
    urchin-image
    urchin-motor-sim
    qemu-esp32
  ];
}
