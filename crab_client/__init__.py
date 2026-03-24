from __future__ import annotations

from pathlib import Path

_PKG_DIR = Path(__file__).resolve().parent
_SRC_DIR = _PKG_DIR / "src"

if str(_SRC_DIR) not in __path__:
    __path__.append(str(_SRC_DIR))

with (_SRC_DIR / "__init__.py").open("rb") as _handle:
    exec(compile(_handle.read(), str(_SRC_DIR / "__init__.py"), "exec"))
