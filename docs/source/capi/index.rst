C++ API
=======

.. raw:: latex

   \setcounter{codelanguage}{1}

ReadMPO library
---------------

.. doxysummary::
   :toctree: generated

   readmpo::MasterMpo
   readmpo::SingleMpo
   readmpo::NdArray
   readmpo::query_mpo
   readmpo::XsType

ReadMPO executable
------------------

The executable will read a list of MPO files and return a serialized data of retrieved cross section. For example, to
save macroscopic cross section of U235, U238 and reactions Absorption and NuFission inside the ``output`` folder:

.. code-block:: sh

   readmpo -i U235 -i U238 -r Absorption -r NuFission -o ./output/ -sk time -xs 1 -g "flxh_FA_aro_6th_GEO" -e "grp002_ENE" /path/to/mpo/files/*.hdf

If the reaction ``Diffusion`` is provided, the order of anisotropy is ``0`` by default. To get a specific anisotropy
order, the argument ``-ao ...`` must be provided:

.. code-block:: sh

   readmpo -i U235 -r Diffusion -ao 1 -sk time -g "flxh_FA_aro_6th_GEO" -e "grp002_ENE" /path/to/mpo/files/*.hdf

The binary output file is formatted as follow:

-  The first ``8`` bytes is an ``std::uint64_t`` indicating ``ndim``, the number of dimension of the array.

-  The next ``ndim * 8`` bytes indicate the shape on each dimension of the dataset. Each shape is saved in the form of
   a ``std::uint64_t``.

-  The rest of the bytes is cross sectional data in the form of a ``double`` array of C-contiguous order (elements with
   consecutive last index are placed next to each other).
