// Copyright 2023 quocdang1998
#ifndef READMPO_ND_ARRAY_HPP_
#define READMPO_ND_ARRAY_HPP_

#include <cstdint>  // std::uint64_t
#include <string>   // std::string
#include <utility>  // std::forward, std::move
#include <vector>   // std::vector

namespace readmpo {

/** @brief C-contiguous multi-dimensional array.*/
class NdArray {
  public:
    /// @name Constructors
    /// @{
    /** @brief Default constructor.*/
    NdArray(void) = default;
    /** @brief Constructor of an zero-filled array from its shape.*/
    NdArray(const std::vector<std::uint64_t> & shape);
    /** @brief Constructor from buffer protocol.*/
    NdArray(double * data, std::vector<std::uint64_t> && shape, std::vector<std::uint64_t> && strides);
    /// @}

    /// @name Copy and move
    /// @{
    /** @brief Copy constructor.*/
    NdArray(const NdArray & src);
    /** @brief Copy assignment.*/
    NdArray & operator=(const NdArray & src);
    /** @brief Move constructor.*/
    NdArray(NdArray && src);
    /** @brief Move assignment.*/
    NdArray & operator=(NdArray && src);
    /// @}

    /// @name Attributes
    /// @{
    /** @brief Get pointer to data.*/
    const double * data(void) const noexcept { return this->data_; }
    /** @brief Get number of dimensions.*/
    std::uint64_t ndim(void) const noexcept { return this->shape_.size(); }
    /** @brief Get constant reference to the shape vector.*/
    constexpr const std::vector<std::uint64_t> & shape(void) const noexcept { return this->shape_; }
    /** @brief Get constant reference to the strides vector.*/
    constexpr const std::vector<std::uint64_t> & strides(void) const noexcept { return this->strides_; }
    /// @}

    /// @name Slicing operator
    /// @{
    /** @brief Get reference to an element by C-contiguous index.*/
    double & operator[](std::uint64_t index);
    /** @brief Get constant reference to an element by C-contiguous index.*/
    const double & operator[](std::uint64_t index) const;
    /** @brief Get reference to an element by multi-dimensional index.*/
    double & operator[](const std::vector<std::uint64_t> & index);
    /** @brief Get constant reference to an element by multi-dimensional index.*/
    const double & operator[](const std::vector<std::uint64_t> & index) const;
    /// @}

    /// @name Representation
    /// @{
    /** @brief String representation.*/
    std::string str(void) const;
    /// @}

    /// @name Serialization
    /// @{
    /** @brief Write data in form of a Stock file to storage.*/
    void serialize(const std::string & fname) const;
    /// @}

    /// @name Destructor
    /// @{
    /** @brief Destructor.*/
    ~NdArray(void);
    /// @}

  protected:
    /** @brief Pointer to its underlying data.*/
    double * data_ = nullptr;
    /** @brief Number of elements.*/
    std::uint64_t size_ = 0;
    /** @brief De-allocate memory in destructor.*/
    bool free = true;
    /** @brief Shape vector.*/
    std::vector<std::uint64_t> shape_;
    /** @brief Stride vector.*/
    std::vector<std::uint64_t> strides_;
};

}  // namespace readmpo

#endif  // READMPO_ND_ARRAY_HPP_
