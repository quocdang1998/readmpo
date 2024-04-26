// Copyright 2023 quocdang1998
#include "readmpo/nd_array.hpp"

#include <cstring>    // std::memset
#include <fstream>    // std::ofstream
#include <sstream>    // std::ostringstream
#include <stdexcept>  // std::invalid_argument

namespace readmpo {

// Get leap
static std::uintptr_t get_leap(std::uint64_t index, const std::vector<std::uint64_t> & shape,
                               const std::vector<std::uint64_t> & strides) {
    std::uint64_t cum_prod = 1, nd_index = 0, ndim = shape.size();
    std::uintptr_t leap = 0;
    for (std::int64_t i_dim = ndim - 1; i_dim >= 0; i_dim--) {
        nd_index = (index / cum_prod) % shape[i_dim];
        leap += strides[i_dim] * nd_index;
        cum_prod *= shape[i_dim];
    }
    return leap;
}

// Constructor from shape
NdArray::NdArray(const std::vector<std::uint64_t> & shape) : shape_(shape), strides_(shape.size()) {
    // calculate number of element and allocate memory
    this->size_ = 1;
    for (std::uint64_t i = 0; i < this->ndim(); i++) {
        this->size_ *= shape[i];
    }
    this->data_ = new double[this->size_];
    std::memset(this->data_, 0, this->size_ * sizeof(double));
    // calculate stride vector
    std::uint64_t cumprod = sizeof(double);
    for (std::int64_t i = this->ndim() - 1; i >= 0; i--) {
        this->strides_[i] = cumprod;
        cumprod *= shape[i];
    }
}

// Constructor from buffer protocol
NdArray::NdArray(double * data, std::vector<std::uint64_t> && shape, std::vector<std::uint64_t> && strides) :
data_(data), shape_(std::move(shape)), strides_(std::move(strides)), free(false) {
    // calculate number of element
    this->size_ = 1;
    for (std::uint64_t i = 0; i < this->ndim(); i++) {
        this->size_ *= this->shape_[i];
    }
}

// Copy constructor
NdArray::NdArray(const NdArray & src) : shape_(src.shape_), strides_(src.ndim()), size_(src.size_) {
    // copy data
    this->data_ = new double[src.size_];
    for (std::uint64_t i = 0; i < src.size_; i++) {
        this->data_[i] = src[i];
    }
    // calculate strides
    std::uint64_t cumprod = sizeof(double);
    for (std::int64_t i = this->ndim() - 1; i >= 0; i--) {
        this->strides_[i] = cumprod;
        cumprod *= this->shape_[i];
    }
}

// Copy assignment
NdArray & NdArray::operator=(const NdArray & src) {
    // free current data
    if ((this->data_ != nullptr) && this->free) {
        delete[] this->data_;
    }
    // direct assignment
    this->shape_ = src.shape_;
    this->size_ = src.size_;
    this->free = true;
    // copy data
    this->data_ = new double[src.size_];
    for (std::uint64_t i = 0; i < src.size_; i++) {
        this->data_[i] = src[i];
    }
    // calculate strides
    this->strides_ = std::vector<std::uint64_t>(src.ndim());
    std::uint64_t cumprod = sizeof(double);
    for (std::int64_t i = this->ndim() - 1; i >= 0; i--) {
        this->strides_[i] = cumprod;
        cumprod *= this->shape_[i];
    }
    return *this;
}

// Move constructor
NdArray::NdArray(NdArray && src) :
shape_(std::forward<std::vector<std::uint64_t>>(src.shape_)),
strides_(std::forward<std::vector<std::uint64_t>>(src.strides_)),
free(src.free) {
    std::swap(this->size_, src.size_);
    std::swap(this->data_, src.data_);
}

// Move assignment
NdArray & NdArray::operator=(NdArray && src) {
    this->shape_ = std::move(src.shape_);
    this->strides_ = std::move(src.strides_);
    this->free = src.free;
    this->size_ = std::exchange(src.size_, 0);
    this->data_ = std::exchange(src.data_, nullptr);
    return *this;
}

// Get reference to an element by C-contiguous index
double & NdArray::operator[](std::uint64_t index) {
    std::uintptr_t destination = reinterpret_cast<std::uintptr_t>(this->data_);
    destination += get_leap(index, this->shape_, this->strides_);
    return *(reinterpret_cast<double *>(destination));
}

// Get constant reference to an element by C-contiguous index
const double & NdArray::operator[](std::uint64_t index) const {
    std::uintptr_t destination = reinterpret_cast<std::uintptr_t>(this->data_);
    destination += get_leap(index, this->shape_, this->strides_);
    return *(reinterpret_cast<double *>(destination));
}

// Get reference to an element by multi-dimensional index
double & NdArray::operator[](const std::vector<std::uint64_t> & index) {
    if (index.size() != this->ndim()) {
        throw std::invalid_argument("Index must have the same dimension as the array.\n");
    }
    std::uintptr_t destination = reinterpret_cast<std::uintptr_t>(this->data_);
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
    std::uintptr_t destination = reinterpret_cast<std::uintptr_t>(this->data_);
    for (std::uint64_t i = 0; i < this->ndim(); i++) {
        destination += index[i] * this->strides_[i];
    }
    return *(reinterpret_cast<double *>(destination));
}

// String representation
std::string NdArray::str(void) const {
    std::ostringstream os;
    os << "<NdData(";
    for (std::uint64_t i = 0; i < this->size_; i++) {
        os << ((i != 0) ? " " : "");
        os << this->operator[](i);
    }
    os << ")>";
    return os.str();
}

// Write data in form of a Stock file to storage
void NdArray::serialize(const std::string & fname) const {
    std::ofstream outfile(fname.c_str(), std::ios_base::binary | std::ios_base::trunc);
    if (!outfile) {
        throw std::invalid_argument("Cannot open file " + fname + "\n");
    }
    std::uint64_t ndim = this->ndim();
    outfile.write(reinterpret_cast<char *>(&ndim), sizeof(std::uint64_t));
    outfile.write(reinterpret_cast<const char *>(this->shape_.data()), ndim * sizeof(std::uint64_t));
    // outfile.write(reinterpret_cast<const char *>(this->data_.data()), this->data_.size() * sizeof(double));
    for (std::uint64_t i = 0; i < this->size_; i++) {
        const double * p_elem = &(this->operator[](i));
        outfile.write(reinterpret_cast<const char *>(p_elem), sizeof(double));
    }
    outfile.close();
}

// Destructor
NdArray::~NdArray(void) {
    if ((this->data_ != nullptr) && this->free) {
        delete[] this->data_;
    }
}

}  // namespace readmpo
