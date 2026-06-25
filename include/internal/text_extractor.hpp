#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace foundation::text_extractor {

std::vector<std::string> Parse(std::span<const std::byte> pdf_bytes);

std::vector<std::string> ParseWithKey(
    std::span<const std::byte> pdf_bytes,
    std::span<const std::byte> file_key);

std::string ToCanonical(const std::vector<std::string>& pages);

}  // namespace foundation::text_extractor
