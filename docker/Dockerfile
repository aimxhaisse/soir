# Soir documentation website - self-contained image.
#
# Build context is the repository root (see docker/docker-compose.yml), so
# `.dockerignore` at the repo root (symlinked to .gitignore) applies and the
# context stays lean (no build/, _deps/, .venv/, *.so, .git/).
#
#   docker compose -f docker/docker-compose.yml up --build

# --- Builder: compile the C++ extension and produce a wheel -----------------
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -q -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        g++ \
        pkg-config \
        curl \
        ca-certificates \
        python3 \
    && rm -rf /var/lib/apt/lists/*

# uv (static binary) + just
RUN curl -LsSf https://astral.sh/uv/install.sh | sh
ENV PATH="/root/.local/bin:$PATH"
RUN curl -LsSf https://just.systems/install.sh | bash -s -- --to /usr/local/bin

WORKDIR /soir
COPY . .

# setup installs free-threaded Python 3.14.2t + creates the venv;
# wheel runs the CMake/CPack-independent PEP 517 build into dist/*.whl.
RUN just setup \
    && just wheel

# --- Runtime: lean web server -----------------------------------------------
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Runtime native deps of the compiled _bindings extension (headless, no GUI).
RUN apt-get update && apt-get install -q -y --no-install-recommends \
        ca-certificates \
        libjack-jackd2-0 \
        libasound2 \
        libx11-6 \
        libxcb1 \
        libdb5.3 \
        libfontconfig1 \
    && rm -rf /var/lib/apt/lists/*

# Reuse the static uv binary from the builder to avoid a second download.
COPY --from=builder /root/.local/bin/uv /usr/local/bin/uv
ENV PATH="/usr/local/bin:$PATH"

# Free-threaded Python 3.14.2t in a relocatable venv (mirrors `just setup`).
RUN uv python install 3.14.2+freethreaded \
    && uv venv --python "$(uv python find 3.14.2+freethreaded)" /opt/venv
ENV VIRTUAL_ENV=/opt/venv
ENV PATH="/opt/venv/bin:$PATH"

# Install the wheel (pulls in flask/gunicorn/typer/... and the .so binding).
COPY --from=builder /soir/dist/*.whl /tmp/
RUN uv pip install --no-cache /tmp/*.whl && rm -f /tmp/*.whl

# Match the `soir` launcher: run GIL-disabled and use a persistent data dir.
ENV SOIR_HOME=/data
ENV PYTHON_GIL=0
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

WORKDIR /app

EXPOSE 5000

HEALTHCHECK --interval=30s --timeout=10s --start-period=20s --retries=3 \
    CMD /opt/venv/bin/python -c "import urllib.request,sys; sys.exit(0 if urllib.request.urlopen('http://127.0.0.1:5000/', timeout=5).status==200 else 1)"

CMD ["gunicorn", \
    "--bind", "0.0.0.0:5000", \
    "--workers", "2", \
    "--access-logfile", "-", \
    "--error-logfile", "-", \
    "soir.www.app:create_app()"]
