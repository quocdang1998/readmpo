ReadMPO
=======

[![Language](https://img.shields.io/badge/language-C++20-0076fc)](https://en.cppreference.com/w/cpp/20)
[![Language](https://img.shields.io/badge/language-Python%3E%3D3.6-0076fc)](https://www.python.org/)

Overview
--------

Library for decoding and extracting data from MPO libraries resulted from APOLLO3 lattice calculations involving
homogenization in space and condensation in energy. Retrieved information, such as microscopic and macroscopic cross
sections, zone flux, and reaction rates, are saved in a serialized format (through the C++ executable), or in a Python
dictionary format (using the Python API).

Installation
------------

To compile C++ API, execute:

```
cmake --preset=linux .  # "cmake --preset=windows ." on Windows
cd build
make -j  # "ninja" on Windows
```

To compile Python library in source directory, execute:

```
python setup.py build_ext --inplace
```

To install Python library, execute:

```
pip install .
```

To compile documentation:

```
cd docs
doxygen Doxyfile
make html
```

Contact
-------

Please contact [Dinh Quoc Dang Nguyen](mailto:quocdang1998@gmail.com) for bugs.
