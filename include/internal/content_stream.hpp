#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "objects.hpp"

namespace foundation::content_stream {

struct Operation {
    std::string op;
    std::vector<objects::Value> operands;
};

struct Stream {
    std::vector<Operation> operations;
};

Stream Parse(std::span<const std::byte> input);
std::string ToCanonical(const Stream& stream);

}
