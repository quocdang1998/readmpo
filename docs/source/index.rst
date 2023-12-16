.. readmpo documentation master file, created by
   sphinx-quickstart on Sat Dec 16 09:03:17 2023.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to ReadMPO's documentation!
===================================

.. raw:: latex

   \chapter{Introduction}

ReadMPO is a library for decoding and retrieving data from MPO libraries resulted from APOLLO3 lattice calculations using
homogenization in space and condensation in energy. Microscopic and macroscopic cross section, as well as zone flux and
reaction rate are saved in a serialized format (using C++ executable), or a Python dictionary format (using Python API).

.. toctree::
   :maxdepth: 1

   installation
   capi/index
   pyapi/index
