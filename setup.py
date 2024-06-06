# Copyright 2023 quocdang1998
import os
import sys
import numpy as np

from jinja2 import Template
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
ext_options["library_dirs"] += [H5_LIB]
if sys.platform == "linux":
    ext_options["extra_compile_args"] = ["-std=c++20", "-flto=auto", "-fno-fat-lto-objects"]
    ext_options["depends"] = ["readmpo/main.cpp", "build/libreadmpo.a"]
    ext_options["runtime_library_dirs"] = [H5_LIB]
elif sys.platform == "win32":
    ext_options["extra_compile_args"] = ["/std:c++20"]
    ext_options["depends"] = ["readmpo/main.cpp", "build/readmpo.lib"]

# build extension
readmpo_extensions = [
    Pybind11Extension("readmpo.clib", ["readmpo/main.cpp"], **ext_options)
]

# create __init__.py
if sys.platform == "win32":
    init_template_txt = "import os\nos.add_dll_directory(\"{{ h5_bin }}\")\n"
else:
    init_template_txt = ""
init_template_txt += "from readmpo.clib import *\n"
init_template = Template(init_template_txt)
template_dict = {
    "h5_bin": H5_BIN
}

if __name__ == "__main__":
    with open(os.path.join(os.path.curdir, "readmpo/__init__.py"), "w") as init_file:
        init_file.write(init_template.render(template_dict))

    setup(name="readmpo",
          version="1.0.0",
          author="quocdang1998",
          author_email="quocdang1998@gmail.com",
          packages=["readmpo"],
          ext_modules=readmpo_extensions,
          python_requires=">=3.6",
          install_requires=["numpy>1.19"]
    )
