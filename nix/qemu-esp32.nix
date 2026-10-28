# Espressif's QEMU fork (https://github.com/espressif/qemu/releases).
{
  lib,
  stdenv,
  fetchurl,
  autoPatchelfHook,
  pixman,
  libgcrypt,
  SDL2,
  zlib,
  libslirp,
}:
let
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
  system = stdenv.hostPlatform.system;
in
stdenv.mkDerivation {
  pname = "qemu-esp-xtensa";
  inherit version;
  src = fetchurl {
    url = "https://github.com/espressif/qemu/releases/download/${tag}/qemu-xtensa-softmmu-esp_develop_${
      builtins.replaceStrings [ "-" ] [ "_" ] version
    }-${triples.${system}}.tar.xz";
    sha256 = hashes.${system};
  };
  nativeBuildInputs = [ autoPatchelfHook ];
  buildInputs = [
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
    platforms = builtins.attrNames hashes;
  };
}
