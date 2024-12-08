_default:
    @just --list --unsorted

# --- Builder/development part
#
# Goal is to get rid of the makefile at some point and only rely on
# this justfile.

# Build Soir.
build:
    #!/usr/bin/env bash
    poetry run make

# Runs the Soir unit test suites.
test filter='*':
    poetry run make test TEST_FILTER={{filter}}

# Build and push documentation to soir.sbrk.org.
doc:
    poetry run make push

# --- User/session part
#
# To be migrated to a dedicated justfile that will be the entrypoint
# to performances/initialization/setup.

sample_dir := "assets/samples"

# Runs Soir.
run:
    poetry run ./bin/soir

# Runs Soir measuring perf.
perf-record:
    #!/usr/bin/env bash
    perf record -p $(pidof soir)

# Rebuilds all sample packs from the samples directory.
mk-sample-packs force="True":
    #!/usr/bin/env -S poetry run python3
    import yaml
    import os
    import sys

    def normalize_name(name: str) -> str:
        """Normalize a name to be used as a sample name in Soir.
        """
        for char in ["_", "(", ")", "[", "]", "{", "}", ":", ";", ",", ".", "!", "?", " "]:
            name = name.replace(char, "-")
        return name.lower()

    def get_samples_from_directory(directory: str) -> list:
        """Get all samples from a directory.
        """
        samples = []
        names = set()
        for root, dirs, files in os.walk(directory):
            for file in files:
                if file.endswith(".wav"):
                    full_path = os.path.join(root, file)
                    name = normalize_name(os.path.splitext(file)[0])
                    if name in names:
                        raise ValueError(f"Collision in sample names: {name} already exist")
                    names.add(name)
                    samples.append({'name': name, 'path': full_path})
        return samples

    def get_sample_directories(directory: str) -> list[str]:
        """Get all directories containing sample packs.
        """
        return [os.path.join(directory, d) for d in os.listdir(directory) if os.path.isdir(os.path.join(directory, d))]

    def main():
        """Main function.
        """
        sample_dirs = get_sample_directories("{{sample_dir}}")
        for sample_dir in sample_dirs:
            pack_file = f"{sample_dir}.pack.yaml"
            if os.path.exists(pack_file) and not {{force}}:
                print(f"Pack file {pack_file} already exists, skipping")
                continue
            samples = get_samples_from_directory(sample_dir)
            with open(pack_file, "w") as f:
                data = {'name': sample_dir, 'samples': samples}
                yaml.dump(data, f)
                print(f"Pack file {pack_file} created")

    main()
