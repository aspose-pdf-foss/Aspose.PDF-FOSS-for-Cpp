#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace foundation::trailer {

struct Trailer {
    std::uint32_t size;
    std::uint32_t root_id;
    std::uint32_t info_id;
    std::uint32_t encrypt_id;
};

Trailer Parse(std::span<const std::byte> input);
std::string ToCanonical(const Trailer& trailer);

}
