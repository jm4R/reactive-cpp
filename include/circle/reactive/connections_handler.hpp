#pragma once

#include <circle/reactive/signal.hpp>

#include <vector>

namespace circle {

class connections_handler
{
protected:
    std::vector<circle::scoped_connection> circle_connections_;
};

} // namespace circle

#define AUTO_CONNECT(signal_or_prop, method, ...)                              \
    this->circle_connections_.emplace_back(signal_or_prop.connect(             \
        &std::decay_t<decltype(*this)>::method, this, ##__VA_ARGS__))
