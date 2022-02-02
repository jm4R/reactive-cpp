#pragma once

#include <circle/bind/property.hpp>

namespace circle {

template <typename T>
class property_ref
{
public:
    property_ref(property<T>& prop) : ptr_{&prop} { rebind(); }

private:
    void rebind() {}

private:
    property<T>* ptr_;
};

} // namespace circle