Python API
==========

.. raw:: latex

   \setcounter{codelanguage}{0}

Python API contains wrapper class around C++ API.

To decode cross section data for a list of isotopes and reactions:

.. code-block:: py

   import glob
   import numpy as np
   from readmpo import MasterMpo, XsType

   # initialize a master MPO containing all MPOs
   master_mpo = MasterMpo(
       mpofile_list=glob.glob("/path/to/mpo/files/*.hdf"),
       geometry="flxh_FA_aro_6th_GEO",
       energy_mesh="grp002_ENE",
   )

   # retrieve a list of isotopes and reactions
   macrolib = master_mpo.build_microlib_xs(
       isotopes=["U235", "U238"],
       reactions=["Absorption", "Diffusion", "Scattering", "NuFission"],
       skipped_dims=["time"],
       type=XsType.Macro,
       max_anisop_order=2,  # retrieve order 0 and 1
       log_file="log.txt"
   )

   # convert retrieved data to Numpy array without copy
   u235_abs_macro = np.array(macrolib["U235"]["Absorption"], copy=False)


Members of the Python module:

.. autosummary::
   :toctree: generated
   :nosignatures:
   :template: pyclass.rst

   readmpo.MasterMpo
   readmpo.SingleMpo
   readmpo.query_mpo
   readmpo.NdArray
