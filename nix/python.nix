# Python environment for the Crab API.
{ python3 }:
let
  # Not packaged in nixpkgs; required by Crab_API (see Crab_API/pyproject.toml).
  python = python3.override {
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
in
python.withPackages (
  ps: with ps; [
    pyserial
    tomlkit
    tomlkit-extras
    pytest
  ]
)
