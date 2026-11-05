# Build from the repo root so zaraos.json is in the build context:
#   docker build -f Crab_API2/Dockerfile -t crab-api2:dev .
# For the simulator, also pass --build-arg URCHIN_PORT=socket://<host-ip>:5555
FROM python:3.11-slim


# PYTHONUNBUFFERED: show print() output in Cortex logs right away.
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /usr/local/bin/
ENV UV_LINK_MODE=copy \
    UV_COMPILE_BYTECODE=1 \
    UV_PYTHON_DOWNLOADS=never \
    URCHIN_PORT=socket://10.0.2.2:5555

WORKDIR /app 

COPY Crab_API2/pyproject.toml Crab_API2/README.md ./
COPY Crab_API2/src ./
RUN uv pip install --system --no-cache .
COPY Crab_API2/examples ./examples

ARG URCHIN_PORT=""
ENV URCHIN_PORT=${URCHIN_PORT}
COPY zaraos.json /etc/zaraos/package.json
CMD [ "python", "examples/demo.py" ]