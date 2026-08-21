# Docker deployment (www)

Builds a self-contained image that compiles the C++ extension in a builder
stage and runs the Soir documentation website (gunicorn + Flask) in a lean
runtime stage. No local toolchain is required on the server — the whole build
happens inside the container.

## Usage

```bash
# Build and start (detached)
docker compose -f docker/docker-compose.yml up -d --build

# Stop and remove containers (keeps the data volume)
docker compose -f docker/docker-compose.yml down
```

The site is served on **host port 10011** → container port 5000:

```
http://<server>:10011/
```

## Notes

- **Data dir**: `$SOIR_HOME` maps to `/data`, backed by the named volume
  `soir-data`. Remove it with
  `docker volume rm $(docker volume ls -qf NAME=soir-data)`
  (the volume is namespaced by project, e.g. `<project>_soir-data`) to reset
  server state.
- **Python**: the image installs free-threaded CPython 3.14.2t via `uv`
  (`uv python install 3.14.2+freethreaded`) and runs with `PYTHON_GIL=0`,
  matching the `soir` launcher's behaviour.
- **Build context** is the repository root; `.dockerignore` (→ `.gitignore`)
  keeps `build/`, `_deps/`, `.venv/`, `*.so` and VCS files out of the image.
- **Healthcheck** hits `http://127.0.0.1:5000/` via the venv Python (no curl
  needed in the runtime image).
