# 文件路徑: benchmarks/conanfile.py

from conan import ConanFile

class BenchmarksConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("benchmark/1.8.3")