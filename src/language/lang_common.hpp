#ifndef LANG_COMMON_HPP
#define LANG_COMMON_HPP

#include "common.hpp"

struct Variable {
    String name = {};
    String type_name = {};

    Variable() {}
    Variable(String n, String p_type) : name(n), type_name(p_type) {}

    bool operator==(const Variable& other) const {
        return type_name == other.type_name && name == other.name;
    }
};

#endif // LANG_COMMON_HPP
