// Copyright 2023 quocdang1998
#ifndef READMPO_SERIALIZER_HPP_
#define READMPO_SERIALIZER_HPP_

#include <cstdint>  // std::uint32_t
#include <istream>  // std::istream
#include <ostream>  // std::ostream
#include <map>      // std::map
#include <string>   // std::string
#include <tuple>    // std::tuple
#include <type_traits>  // std::is_trivially_copyable
#include <unordered_set>  // std::unordered_set
#include <utility>  // std::pair
#include <vector>  // std::vector

namespace readmpo {

// Serialize
// ---------

/** @brief Serialize a class to an output stream.*/
template <class T>
void serialize_obj(std::ostream & os, const T & obj);

/** @brief Serialize a pair.*/
template <class F, class S>
void serialize_obj(std::ostream & os, const std::pair<F, S> & obj);

/** @brief Serialize a tuple.*/
template <typename... Args>
void serialize_obj(std::ostream & os, const std::tuple<Args...> & obj);

/** @brief Serialize a string.*/
template <>
void serialize_obj(std::ostream & os, const std::string & obj);

/** @brief Serialize a vector.*/
template <class T>
void serialize_obj(std::ostream & os, const std::vector<T> & obj);

/** @brief Serialize an unordered set.*/
template <class T>
void serialize_obj(std::ostream & os, const std::unordered_set<T> & obj);

/** @brief Serialize a map.*/
template <class K, class T>
void serialize_obj(std::ostream & os, const std::map<K, T> & obj);

// Deserialize
// -----------

/** @brief Deserialize a class to an output stream.*/
template <class T>
void deserialize_obj(std::istream & is, T & obj);

/** @brief Deserialize a pair.*/
template <class F, class S>
void deserialize_obj(std::istream & is, std::pair<F, S> & obj);

/** @brief Deserialize a tuple.*/
template <typename... Args>
void deserialize_obj(std::istream & is, std::tuple<Args...> & obj);

/** @brief Deserialize a string.*/
template <>
void deserialize_obj(std::istream & is, std::string & obj);

/** @brief Deserialize a vector.*/
template <class T>
void deserialize_obj(std::istream & is, std::vector<T> & obj);

/** @brief Deserialize an unordered set.*/
template <class T>
void deserialize_obj(std::istream & is, std::unordered_set<T> & obj);

/** @brief Deserialize a map.*/
template <class K, class T>
void deserialize_obj(std::istream & is, std::map<K, T> & obj);

}  // namespace readmpo

#include "readmpo/serializer.tpp"

#endif  // READMPO_SERIALIZER_HPP_
