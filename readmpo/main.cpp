// Copyright 2023 quocdang1998
#include "readmpo/master_mpo.hpp"  // readmpo::MasterMpo
#include "readmpo/nd_array.hpp"    // readmpo::NdArray
#include "readmpo/query_mpo.hpp"   // readmpo::query_mpo
#include "readmpo/single_mpo.hpp"  // readmpo::SingleMpo

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

namespace py = pybind11;

namespace readmpo {

// Wrap ``readmpo::NdArray`` class
void wrap_nd_array(py::module & readmpo_package) {
    auto nd_array_pyclass = py::class_<NdArray>(
        readmpo_package,
        "NdArray",
        py::buffer_protocol(),
        R"(
        C-contiguous multi-dimensional array on CPU.
        )"
    );
    // constructors
    nd_array_pyclass.def(
        py::init([]() { return new NdArray(); }),
        "Default constructor."
    );
    nd_array_pyclass.def(
        py::init(
            [](py::list & shape) {
                std::vector<std::uint64_t> shape_cpp = shape.cast<std::vector<std::uint64_t>>();
                return new NdArray(shape_cpp);
            }
        ),
        "Construct zero-filled C-contiguous array from dimension vector.",
        py::arg("shape")
    );
    // conversion to Numpy
    nd_array_pyclass.def_buffer(
        [](NdArray & self) {
            return py::buffer_info(const_cast<double *>(self.data()), sizeof(double),
                                   py::format_descriptor<double>::format(), self.ndim(), self.shape(), self.strides());
        }
    );
}

// Wrap ``readmpo::XsType`` enum
void wrap_xstype(py::module & readmpo_package) {
    auto xstype_pyenum = py::enum_<XsType>(
        readmpo_package,
        "XsType",
        "Wrapper of :cpp:enum:`readmpo::XsType`"
    );
    xstype_pyenum.value("Micro", XsType::Micro);
    xstype_pyenum.value("Macro", XsType::Macro);
    xstype_pyenum.value("Flux", XsType::Flux);
    xstype_pyenum.value("ReactRate", XsType::ReactRate);
}

// Wrap ``readmpo::SingleMpo`` class
void wrap_single_mpo(py::module & readmpo_package) {
    auto single_mpo_pyclass = py::class_<SingleMpo>(
        readmpo_package,
        "SingleMpo",
        R"(
        Class representing a single output ID inside an MPO.
        )"
    );
    // constructor
    single_mpo_pyclass.def(
        py::init(
            [](const std::string & mpofile_name, const std::string & geometry, const std::string & energy_mesh) {
                return new SingleMpo(mpofile_name, geometry, energy_mesh);
            }
        ),
        "Constructor from list of MPO file names, name of homogenized geometry and name of energy mesh.",
        py::arg("mpofile_name"), py::arg("geometry"), py::arg("energy_mesh")
    );
    // attributes
    single_mpo_pyclass.def_property_readonly(
        "state_params",
        [](SingleMpo & self) { return py::cast(self.get_state_params()); },
        "Get state parameters."
    );
    single_mpo_pyclass.def_property_readonly(
        "isotopes",
        [](SingleMpo & self) { return py::cast(self.get_isotopes()); },
        "Get list of isotopes in MPO."
    );
    single_mpo_pyclass.def_property_readonly(
        "reactions",
        [](SingleMpo & self) { return py::cast(self.get_reactions()); },
        "Get list of reactions in MPO."
    );
    single_mpo_pyclass.def_property_readonly(
        "n_zones",
        [](SingleMpo & self) { return self.n_zones; },
        "Number of zones in the geometry."
    );
    single_mpo_pyclass.def_property_readonly(
        "n_groups",
        [](SingleMpo & self) { return self.n_groups; },
        "Number of groups in the energy mesh."
    );
    // representation
    single_mpo_pyclass.def(
        "__repr__",
        [](const SingleMpo & self) { return self.str(); }
    );
}

// Wrap ``readmpo::MasterMpo`` class
void wrap_master_mpo(py::module & readmpo_package) {
    auto master_mpo_pyclass = py::class_<MasterMpo>(
        readmpo_package,
        "MasterMpo",
        R"(
        Class containing merged information of all MPOs.
        )"
    );
    // constructor
    master_mpo_pyclass.def(
        py::init(
            [](py::list & mpofile_pylist, const std::string & geometry, const std::string & energy_mesh) {
                std::vector<std::string> mpofile_list = mpofile_pylist.cast<std::vector<std::string>>();
                return new MasterMpo(mpofile_list, geometry, energy_mesh);
            }
        ),
        "Constructor from list of MPO file names, name of homogenized geometry and name of energy mesh.",
        py::arg("mpofile_list"), py::arg("geometry"), py::arg("energy_mesh")
    );
    // attributes
    master_mpo_pyclass.def_property_readonly(
        "master_pspace",
        [](MasterMpo & self) { return py::cast(self.master_pspace()); },
        "Get merged parameter space."
    );
    master_mpo_pyclass.def_property_readonly(
        "isotopes",
        [](MasterMpo & self) { return py::cast(self.get_isotopes()); },
        "Get available isotopes."
    );
    master_mpo_pyclass.def_property_readonly(
        "reactions",
        [](MasterMpo & self) { return py::cast(self.get_reactions()); },
        "Get available reactions."
    );
    // get data
    master_mpo_pyclass.def(
        "build_microlib_xs",
        [](MasterMpo & self, py::list & isotopes_list, py::list & reactions_list, py::list & skipped_dims_list,
           XsType type, std::uint64_t anisotropy_order) {
            std::vector<std::string> isotopes = isotopes_list.cast<std::vector<std::string>>();
            std::vector<std::string> reactions = reactions_list.cast<std::vector<std::string>>();
            std::vector<std::string> skipped_dims = skipped_dims_list.cast<std::vector<std::string>>();
            auto microlib = self.build_microlib_xs(isotopes, reactions, skipped_dims, type, anisotropy_order);
            py::dict result = py::cast(microlib);
            return result;
        },
        "Retrieve microscopic homogenized cross sections at some isotopes, reactions and skipped dimensions in all MPO files.",
        py::arg("isotopes"), py::arg("reactions"), py::arg("skipped_dims"), py::arg("type") = XsType::Micro, py::arg("anisotropy_order") = 0
    );
    master_mpo_pyclass.def(
        "get_concentration",
        [](MasterMpo & self, py::list & isotopes_list, const std::string & burnup_name) {
            std::vector<std::string> isotopes = isotopes_list.cast<std::vector<std::string>>();
            auto conclib = self.get_concentration(isotopes, burnup_name);
            py::dict result = py::cast(conclib);
            return result;
        },
        "Retrieve concentration of some isotopes at each value of burnup in each zone.",
        py::arg("isotopes"), py::arg("burnup_name") = "burnup"
    );
}

// Wrap ``readmpo::query_mpo`` function
void wrap_query_mpo(py::module & readmpo_package) {
    readmpo_package.def(
        "query_mpo",
        [] (const std::string & mpofile_name) { return py::cast(query_mpo(mpofile_name)); },
        "Read a MPO file and return geometry names and energy mesh names.",
        py::arg("mpofile_name")
    );
}

}  // namespace readmpo

// Wrap main module
PYBIND11_MODULE(clib, readmpo_package) {
    readmpo_package.doc() = "Python interface of readmpo library.";
    // wrap NdArray
    readmpo::wrap_nd_array(readmpo_package);
    // wrap XsType
    readmpo::wrap_xstype(readmpo_package);
    // wrap SingleMpo
    readmpo::wrap_single_mpo(readmpo_package);
    // wrap MasterMpo
    readmpo::wrap_master_mpo(readmpo_package);
    // wrap query_mpo
    readmpo::wrap_query_mpo(readmpo_package);
}
