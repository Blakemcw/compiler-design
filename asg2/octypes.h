// $Id: octypes.h,v 1.4 2019-09-16 14:36:17-07 - - $

// Type definitiions to compile oc programs with g++.

#ifndef __OCDEFS_H__
#define __OCDEFS_H__

#include <type_traits>

using string = char*;

template <typename type>
using ptr = std::enable_if_t<std::is_class<type>::value,type*>;

template <typename type>
struct array {
   using array_value_type = type;
   type* data {};
   array() = default;
   array (type* that) { data = that; }
   array& operator= (type* that) { data = that; return *this; }
   type& operator[] (int i) { return data[i]; }
};

template <typename type>
std::enable_if_t<std::is_class<type>::value,ptr<type>>
alloc() {
   return new type();
}

template <typename type>
array<typename type::array_value_type>
alloc (int size) {
   auto result = new typename type::array_value_type [size] {};
   using result_t = array<typename type::array_value_type>*;
   return *reinterpret_cast<result_t> (&result);
}

template <typename type>
std::enable_if_t<std::is_same<type,string>::value,string>
alloc (int size) {
   return new char[size] {};
}

#endif

