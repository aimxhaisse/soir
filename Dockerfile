FROM ubuntu:24.04 AS builder

RUN apt update && apt upgrade -q -y

RUN apt install -q -y \
    build-essential \
    python3 \
    curl \
    cmake \
    git \
    g++ \
    pkg-config \
    ffmpeg \
    libasound2-dev \
    libx11-dev \
    libxcb1-dev \
    libxcb-util-dev \
    libxcb-cursor-dev \
    libxcb-keysyms1-dev \
    libxcb-xkb-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libfreetype6-dev \
    libcairo2-dev \
    libfontconfig1-dev \
    libpango1.0-dev \
    libgtkmm-3.0-dev \
    libsqlite3-dev

RUN curl -LsSf https://just.systems/install.sh | bash -s -- --to /usr/bin
RUN curl -LsSf https://astral.sh/uv/install.sh | bash

ENV PATH="/root/.local/bin:$PATH"

WORKDIR /soir
COPY . .

ENV CXXFLAGS="-std=c++17"
ENV CMAKE_CXX_STANDARD=17

RUN just clean
RUN just setup
RUN just build
RUN just test-unit
RUN just test-integration
RUN just package

FROM ubuntu:24.04 AS runtime-www

RUN apt update && apt install -q -y \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /soir

COPY --from=builder /root/.local/share/uv/python /root/.local/share/uv/python
COPY --from=builder /soir/.venv /soir/.venv
COPY --from=builder /soir/py /soir/py
COPY --from=builder /soir/pyproject.toml /soir/pyproject.toml

ENV PATH="/soir/.venv/bin:$PATH"
ENV SOIR_DIR=/soir/data
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1

EXPOSE 5000

HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:5000/ || exit 1

CMD ["gunicorn", \
    "--bind", "0.0.0.0:5000", \
    "--workers", "4", \
    "--access-logfile", "-", \
    "--error-logfile", "-", \
    "soir.www.app:create_app()"]
