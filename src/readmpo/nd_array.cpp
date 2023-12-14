// Copyright 2023 quocdang1998
#include "readmpo/nd_array.hpp"

#include <cstring>    // std::memset
#include <fstream>    // std::ofstream
#include <sstream>    // std::ostringstream
#include <stdexcept>  // std::invalid_argument

namespace readmpo {

// Constructor from shape
NdArray::NdArray(const std::vector<std::uint64_t> & shape) : shape_(shape), strides_(shape.size()) {
    // calculate number of element and allocate memory
    std::uint64_t size = 1;
    for (std::uint64_t i = 0; i < this->ndim(); i++) {
        size *= shape[i];
    }
    this->data_.resize(size);
    std::memset(this->data_.data(), 0, size * sizeof(double));
    // calculate stride vector
    std::uint64_t cumprod = sizeof(double);
    for (std::int64_t i = this->ndim() - 1; i >= 0; i--) {
        this->strides_[i] = cumprod;
        cumprod *= shape[i];
    }
}

// Get reference to an element by multi-dimensional index
double & NdArray::operator[](const std::vector<std::uint64_t> & index) {
    if (index.size() != this->ndim()) {
        throw std::invalid_argument("Index must have the same dimension as the array.\n");
    }
    std::uintptr_t destination = reinterpret_cast<std::uintptr_t>(this->data_.data());
    for (std::uint64_t i = 0; i < this->ndim(); i++) {
        destination += index[i] * this->strides_[i];
    }
    return *(reinterpret_cast<double *>(destination));
}

// Get constant reference to an element by multi-dimensional index
const double & NdArray::operator[](const std::vector<std::uint64_t> & index) const {
    if (index.size() != this->ndim()) {
        throw std::invalid_argument("Index must have the same dimension as the array.\n");
    }
    std::uintptr_t destination = reinterpret_cast<std::uintptr_t>(this->data_.data());
    for (std::uint64_t i = 0; i < this->ndim(); i++) {
        destination += index[i] * this->strides_[i];
    }
    return *(reinterpret_cast<double *>(destination));
}

// String representation
std::string NdArray::str(void) const {
    std::ostringstream os;
    os << "<NdData(";
    for (std::uint64_t i = 0; i < this->data_.size() - 1; i++) {
        os << this->data_[i] << " ";
    }
    os << this->data_[this->data_.size() - 1];
    os << ")>";
    return os.str();
}

// Write data in form of a Stock file to storage
void NdArray::serialize(const std::string & fname) const {
    std::ofstream outfile(fname.c_str(), std::ios_base::binary | std::ios_base::trunc);
    std::uint64_t ndim = this->ndim();
    outfile.write(reinterpret_cast<char *>(&ndim), sizeof(std::uint64_t));
    outfile.write(reinterpret_cast<const char *>(this->shape_.data()), ndim * sizeof(std::uint64_t));
    outfile.write(reinterpret_cast<const char *>(this->data_.data()), this->data_.size() * sizeof(double));
    outfile.close();
}

}  // namespace readmpo
