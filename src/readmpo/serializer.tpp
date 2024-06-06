// Copyright 2024 quocdang1998
#ifndef READMPO_SERIALIZER_TPP_
#define READMPO_SERIALIZER_TPP_

namespace readmpo {

// Serialize a class to an output stream
template <class T>
requires std::is_trivially_copyable<T>::value
void serialize_obj(std::ostream & os, const T & obj) {
    os.write(reinterpret_cast<const char *>(&obj), sizeof(T));
}

// Serialize a pair
template <class F, class S>
void serialize_obj(std::ostream & os, const std::pair<F, S> & obj) {
    serialize_obj(os, obj.first);
    serialize_obj(os, obj.second);
}

// Serialize a tuple
template <typename... Args>
void serialize_obj(std::ostream & os, const std::tuple<Args...> & obj) {
    std::apply([&os](const auto &... ts) { (..., serialize_obj(os, ts)); }, obj);
}

// Serialize a string
template <>
void serialize_obj(std::ostream & os, const std::string & obj) {
    std::uint32_t size = obj.size();
    os.write(reinterpret_cast<char *>(&size), sizeof(std::uint32_t));
    os.write(obj.c_str(), size);
}

// Serialize a vector
template <class T>
void serialize_obj(std::ostream & os, const std::vector<T> & obj) {
    std::uint32_t size = obj.size();
    os.write(reinterpret_cast<char *>(&size), sizeof(std::uint32_t));
    for (const T & element : obj) {
        serialize_obj(os, element);
    }
}

// Serialize an unordered set
template <class T>
void serialize_obj(std::ostream & os, const std::unordered_set<T> & obj) {
    std::uint32_t size = obj.size();
    os.write(reinterpret_cast<char *>(&size), sizeof(std::uint32_t));
    for (const T & element : obj) {
        serialize_obj(os, element);
    }
}

// Serialize a map
template <class K, class T>
void serialize_obj(std::ostream & os, const std::map<K, T> & obj) {
    std::uint32_t size = obj.size();
    os.write(reinterpret_cast<char *>(&size), sizeof(std::uint32_t));
    for (const auto & [key, value] : obj) {
        serialize_obj(os, key);
        serialize_obj(os, value);
    }
}

// Deserialize a class to an output stream
template <class T>
requires std::is_trivially_copyable<T>::value
void deserialize_obj(std::istream & is, T & obj) {
    is.read(reinterpret_cast<char *>(&obj), sizeof(T));
}

// Deserialize a pair
template <class F, class S>
void deserialize_obj(std::istream & is, std::pair<F, S> & obj) {
    deserialize_obj(is, obj.first);
    deserialize_obj(is, obj.second);
}

// Deserialize a tuple
template <typename... Args>
void deserialize_obj(std::istream & is, std::tuple<Args...> & obj) {
    std::apply([&is](auto &... ts) { (..., deserialize_obj(is, ts)); }, obj);
}

// Deserialize a string
template <>
void deserialize_obj(std::istream & is, std::string & obj) {
    std::uint32_t size;
    is.read(reinterpret_cast<char *>(&size), sizeof(std::uint32_t));
    obj.resize(size);
    is.read(&(obj[0]), size);
}

// Deserialize a vector
template <class T>
void deserialize_obj(std::istream & is, std::vector<T> & obj) {
    std::uint32_t size;
    is.read(reinterpret_cast<char *>(&size), sizeof(std::uint32_t));
    obj.resize(size);
    for (T & element : obj) {
        deserialize_obj(is, element);
    }
}

// Deserialize an unordered set
template <class T>
void deserialize_obj(std::istream & is, std::unordered_set<T> & obj) {
    std::uint32_t size;
    is.read(reinterpret_cast<char *>(&size), sizeof(std::uint32_t));
    for (std::uint32_t i_elem = 0; i_elem < size; i_elem++) {
        T element;
        deserialize_obj(is, element);
        obj.insert(std::move(element));
    }
}

// Deserialize a map
template <class K, class T>
void deserialize_obj(std::istream & is, std::map<K, T> & obj) {
    std::uint32_t size;
    is.read(reinterpret_cast<char *>(&size), sizeof(std::uint32_t));
    for (std::uint32_t i_elem = 0; i_elem < size; i_elem++) {
        std::pair<K, T> element;
        deserialize_obj(is, element.first);
        deserialize_obj(is, element.second);
        obj.insert(std::move(element));
    }
}

}  // namespace readmpo

#endif  // READMPO_SERIALIZER_TPP_
