FROM python:3.12-slim

COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /usr/local/bin/

ENV UV_LINK_MODE=copy \
    UV_COMPILE_BYTECODE=1 \
    UV_PYTHON_DOWNLOADS=never

WORKDIR /app

COPY Crab_API/pyproject.toml ./
RUN uv pip install --system --no-cache -r pyproject.toml

COPY Crab_API/src/ ./src/

# Embed ZaraOS package manifest so Cortex auto-discovery can resolve
# device passthrough, dependencies, and health check config.
COPY zaraos.json /etc/zaraos/package.json

WORKDIR /app/src
CMD ["python", "main.py"]
