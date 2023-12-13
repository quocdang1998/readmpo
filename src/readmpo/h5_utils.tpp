// Copyright 2023 quocdang1998
#ifndef READMPO_H5_UTILS_TPP_
#define READMPO_H5_UTILS_TPP_

namespace readmpo {

// ---------------------------------------------------------------------------------------------------------------------
// Utils for HDF5 read
// ---------------------------------------------------------------------------------------------------------------------

// Get data from an HDF dataset in form of an ``std::vector``
template <typename T>
std::pair<std::vector<T>, merlin::intvec> get_dset(H5::Group * group, char const * dset_address) {
    // open dataset
    H5::DataSet dset = group->openDataSet(dset_address);
    // get data shape
    H5::DataSpace dspace = dset.getSpace();
    std::uint64_t ndims = dspace.getSimpleExtentNdims();
    std::vector<hsize_t> shape(ndims);
    dspace.getSimpleExtentDims(shape.data());
    // get size of an element
    std::uint64_t element_size = dset.getDataType().getSize();
    // check size of data if not std::string
    ::H5T_class_t type_class = dset.getTypeClass();
    if constexpr (std::is_same_v<T, std::string>) {
        if (type_class != H5T_STRING) {
            FAILURE(std::runtime_error, "Incorrect type provided to the template.\n");
        }
    } else if constexpr (std::is_integral_v<T>) {
        if (type_class != H5T_INTEGER) {
            FAILURE(std::runtime_error, "Incorrect type provided to the template.\n");
        }
        if (sizeof(T) != element_size) {
            FAILURE(std::runtime_error, "Incorrect integer type provided to the template.\n");
        }
    } else if constexpr (std::is_floating_point_v<T>) {
        if (type_class != H5T_FLOAT) {
            FAILURE(std::runtime_error, "Incorrect type provided to the template.\n");
        }
        if (sizeof(T) != element_size) {
            FAILURE(std::runtime_error, "Incorrect float type provided to the template.\n");
        }
    }
    // read data to a buffer
    std::uint64_t npoint = dspace.getSimpleExtentNpoints();
    std::vector<T> data(npoint);
    if constexpr (std::is_same_v<T, std::string>) {
        // read data to buffer
        std::vector<char> buffer(element_size * npoint);
        dset.read(buffer.data(), dset.getDataType());
        // convert to vector of std::string
        for (int i = 0; i < npoint; i++) {
            data[i].assign(buffer.data() + i * element_size, element_size);
        }
    } else {
        // read data directly
        dset.read(data.data(), dset.getDataType());
    }
    // close dataset
    dset.close();
    // convert shape to intvec
    merlin::intvec data_shape(shape.data(), ndims);
    return std::pair<std::vector<T>, merlin::intvec>(data, data_shape);
}

}  // namespace readmpo

#endif  // READMPO_H5_UTILS_TPP_