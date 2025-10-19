import multiprocessing
import os
import subprocess
from pathlib import Path

from setuptools import Command, Extension, setup
from setuptools.command.build_ext import build_ext

BUILD_DIR = Path(__file__).parent.resolve() / "build" / "cmake"


class CMakeBuild(build_ext):
    """Custom build extension for C++/CMake."""

    user_options = build_ext.user_options + [
        ("with-tests", None, "Build C++ tests"),
    ]

    def initialize_options(self):
        super().initialize_options()
        self.with_tests = False

    def build_extension(self, ext: Extension) -> None:
        """Build the C++ extension using CMake."""
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()

        src_dir = Path(__file__).parent.resolve()

        BUILD_DIR.mkdir(parents=True, exist_ok=True)
        self.build_temp = str(BUILD_DIR)

        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}",
            f"-DPYTHON_EXECUTABLE={os.sys.executable}",
            f"-DPython_EXECUTABLE={os.sys.executable}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        ]

        if self.with_tests:
            cmake_args.append("-DBUILD_TESTING=ON")

        build_args = [f"--parallel={multiprocessing.cpu_count()}"]

        subprocess.run(
            ["cmake", str(src_dir), *cmake_args], cwd=self.build_temp, check=True
        )

        subprocess.run(
            ["cmake", "--build", ".", *build_args], cwd=self.build_temp, check=True
        )


class RunCppTests(Command):
    """Command to run C++ tests without rebuilding."""

    description = "Run C++ tests"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        if not BUILD_DIR.exists():
            raise RuntimeError(
                "Build directory not found. Run 'python setup.py build_ext --with-tests' first."
            )

        print("Running C++ tests...")
        subprocess.run(["ctest", "--verbose"], cwd=str(BUILD_DIR), check=True)


class RunPythonTests(Command):
    """Command to run Python tests using pytest."""

    description = "Run Python tests"
    user_options = []

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        print("Running Python tests...")
        subprocess.run(["pytest"], check=True)


setup(
    ext_modules=[Extension("soir._core", sources=[])],
    cmdclass={
        "build_ext": CMakeBuild,
        "run_cpp_tests": RunCppTests,
        "run_python_tests": RunPythonTests,
    },
)
