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
       reactions=["Absorption", "Diffusion", "NuFission"]
       type=XsType.Macro,
       anisotropy_order=0  # get the zero-th anisotropy order of Diffusion reaction
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
