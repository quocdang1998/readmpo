# Copyright 2023 quocdang1998
import os
import sys
import numpy as np

from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension

# add current directory to path
sys.path.append(os.path.curdir)

from readmpo_config import *

# option for compiling extension
ext_options = {
    "language": "c++",
    "include_dirs": ["src", np.get_include()] + H5_INCLUDE.split(";"),
    "define_macros": [("NPY_NO_DEPRECATED_API", "NPY_1_7_API_VERSION")],
    "libraries": ["readmpo", "hdf5_cpp", "hdf5"],
    "library_dirs": ["build"]
}
H5_LIBDIRS = [os.path.join(os.path.dirname(libpath), "lib") for libpath in H5_INCLUDE.split(";")]
ext_options["library_dirs"] += H5_LIBDIRS
if sys.platform == "linux":
    ext_options["extra_compile_args"] = ["-std=c++20"]
    ext_options["depends"] = ["readmpo/main.cpp", "build/libreadmpo.a"]
elif sys.platform == "win32":
    ext_options["extra_compile_args"] = ["-std:c++20"]
    ext_options["depends"] = ["readmpo/main.cpp", "build/readmpo.lib"]

readmpo_extensions = [
    Pybind11Extension("readmpo.__init__", ["readmpo/main.cpp"], **ext_options)
]

if __name__ == "__main__":
    setup(name="readmpo",
          version="1.0.0",
          author="quocdang1998",
          author_email="quocdang1998@gmail.com",
          packages=["readmpo"],
          ext_modules=readmpo_extensions,
          python_requires=">=3.6",
          install_requires=["numpy>1.19"]
    )
