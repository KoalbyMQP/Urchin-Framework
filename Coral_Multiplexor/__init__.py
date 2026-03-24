from __future__ import annotations

from pathlib import Path
import sys

_PKG_DIR = Path(__file__).resolve().parent
_IMPL_DIR = _PKG_DIR / "coral_multiplexor"
_REPO_ROOT = _PKG_DIR.parent

if str(_REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(_REPO_ROOT))

if str(_IMPL_DIR) not in __path__:
    __path__.append(str(_IMPL_DIR))

DEFAULT_BRIDGE_PATH = (_PKG_DIR / "bridge.toml").resolve()

with (_IMPL_DIR / "__init__.py").open("rb") as _handle:
    exec(compile(_handle.read(), str(_IMPL_DIR / "__init__.py"), "exec"))

__all__ = [*__all__, "DEFAULT_BRIDGE_PATH"]
