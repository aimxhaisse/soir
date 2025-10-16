import multiprocessing
import os
import subprocess
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


class CMakeBuild(build_ext):
    """Custom build extension for C++/CMake."""

    def build_extension(self, ext: Extension) -> None:
        """Build the C++ extension using CMake."""
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()

        src_dir = Path(__file__).parent.resolve()

        persistent_build_dir = src_dir / "build" / "cmake"
        persistent_build_dir.mkdir(parents=True, exist_ok=True)
        self.build_temp = str(persistent_build_dir)

        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            f"-DPYTHON_EXECUTABLE={os.sys.executable}",
            f"-DPython_EXECUTABLE={os.sys.executable}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        ]

        build_args = [f"--parallel={multiprocessing.cpu_count()}"]

        subprocess.run(
            ["cmake", str(src_dir), *cmake_args], cwd=self.build_temp, check=True
        )

        subprocess.run(
            ["cmake", "--build", ".", *build_args], cwd=self.build_temp, check=True
        )


setup(
    ext_modules=[Extension("soir._core", sources=[])],
    cmdclass={"build_ext": CMakeBuild},
)
