#include "ascii85.hpp"
#include <algorithm>
#include <cmath>

namespace {

// PDF 32000-1:2008 §7.4.3: ASCII85 uses digits '!' (33) to 'u' (117)
constexpr int ASCII85_BASE = 85;
constexpr int GROUP_SIZE = 4;
constexpr int ENCODED_GROUP_SIZE = 5;
constexpr std::byte MIN_CHAR{33};  // '!'
constexpr std::byte MAX_CHAR{117}; // 'u'
constexpr std::byte ZERO_SHORTCUT{122}; // 'z'
constexpr std::byte PADDING_CHAR{117};  // 'u' used for implicit padding

// PDF 32000-1:2008 §7.4.3: whitespace characters to skip during decode
bool is_whitespace(std::byte b) {
    return b == std::byte{' '} || b == std::byte{'\t'} || b == std::byte{'\r'} ||
           b == std::byte{'\n'} || b == std::byte{'\f'} || b == std::byte{'\0'};
}

// Convert 5 ASCII85 digits to 32-bit value, handling implicit padding
// Returns the 32-bit value and sets actual_digit_count to the number of valid digits
uint32_t decode_group(const std::byte* digits, int count, int& actual_digit_count) {
    if (count == 0) {
        actual_digit_count = 0;
        return 0;
    }
    
    // PDF 32000-1:2008 §7.4.3: final group with 2..5 chars uses implicit 'u' padding
    // A final group of exactly 1 char is malformed
    if (count == 1) {
        throw std::runtime_error("ASCII85: partial group of length 1 is malformed");
    }
    
    // Compute the value by processing each digit
    uint32_t value = 0;
    for (int i = 0; i < count; ++i) {
        std::byte b = digits[i];
        // PDF 32000-1:2008 §7.4.3: valid range is '!' (33) to 'u' (117)
        int digit_value = static_cast<int>(static_cast<unsigned char>(b)) - 33;
        if (digit_value < 0 || digit_value >= ASCII85_BASE) {
            throw std::runtime_error("ASCII85: invalid character in encoded stream");
        }
        value = value * ASCII85_BASE + digit_value;
    }
    
    // PDF 32000-1:2008 §7.4.3: implicit padding with 'u' (84) for partial groups
    for (int i = count; i < ENCODED_GROUP_SIZE; ++i) {
        value = value * ASCII85_BASE + 84; // 'u' - 33 = 84
    }
    
    actual_digit_count = count;
    return value;
}

// Encode a 32-bit value to 5 ASCII85 characters
void encode_value(uint32_t value, std::byte* out) {
    // PDF 32000-1:2008 §7.4.3: c_i = '!' + ((V / 85^(5-i)) mod 85)
    // Process from most significant to least significant digit
    for (int i = 0; i < ENCODED_GROUP_SIZE; ++i) {
        // Compute divisor: 85^(4-i)
        uint32_t divisor = static_cast<uint32_t>(std::pow(ASCII85_BASE, ENCODED_GROUP_SIZE - 1 - i));
        int digit = (value / divisor) % ASCII85_BASE;
        out[i] = static_cast<std::byte>(33 + digit); // '!' + digit
    }
}

} // anonymous namespace

namespace foundation::ascii85 {

std::vector<std::byte> Encode(std::span<const std::byte> input) {
    std::vector<std::byte> result;
    result.reserve((input.size() + 3) / 4 * 5); // Upper bound
    
    size_t i = 0;
    while (i < input.size()) {
        // PDF 32000-1:2008 §7.4.3: process 4-byte groups
        std::byte group[GROUP_SIZE] = {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
        int group_len = 0;
        
        for (int j = 0; j < GROUP_SIZE && i + j < input.size(); ++j) {
            group[j] = input[i + j];
            group_len++;
        }
        
        // Convert to 32-bit big-endian value
        uint32_t value = (static_cast<uint32_t>(static_cast<unsigned char>(group[0])) << 24) |
                         (static_cast<uint32_t>(static_cast<unsigned char>(group[1])) << 16) |
                         (static_cast<uint32_t>(static_cast<unsigned char>(group[2])) << 8) |
                         (static_cast<uint32_t>(static_cast<unsigned char>(group[3])));
        
        // PDF 32000-1:2008 §7.4.3: zero shortcut only for complete 4-byte groups
        if (value == 0 && group_len == GROUP_SIZE) {
            result.push_back(ZERO_SHORTCUT); // 'z'
        } else {
            // Encode to 5 characters, then truncate for partial groups
            std::byte encoded[ENCODED_GROUP_SIZE];
            encode_value(value, encoded);
            
            // PDF 32000-1:2008 §7.4.3: truncate to (group_len + 1) chars for partial groups
            int output_len = group_len + 1;
            for (int j = 0; j < output_len; ++j) {
                result.push_back(encoded[j]);
            }
        }
        
        i += group_len;
    }
    
    return result;
}

std::vector<std::byte> Decode(std::span<const std::byte> input) {
    std::vector<std::byte> result;
    
    // PDF 32000-1:2008 §7.4.3: empty input is valid
    if (input.empty()) {
        return result;
    }
    
    size_t i = 0;
    size_t n = input.size();
    
    while (i < n) {
        // Skip whitespace
        while (i < n && is_whitespace(input[i])) {
            i++;
        }
        
        if (i >= n) break;
        
        // Check for 'z' shortcut
        if (input[i] == ZERO_SHORTCUT) {
            // PDF 32000-1:2008 §7.4.3: 'z' expands to four 0x00 bytes
            result.push_back(std::byte{0});
            result.push_back(std::byte{0});
            result.push_back(std::byte{0});
            result.push_back(std::byte{0});
            i++;
            continue;
        }
        
        // Collect up to 5 non-whitespace characters for a group
        std::byte group[ENCODED_GROUP_SIZE];
        int group_len = 0;
        
        // First character must be valid
        if (i >= n) break;
        
        std::byte b = input[i];
        // PDF 32000-1:2008 §7.4.3: valid range is '!' (33) to 'u' (117)
        int digit_value = static_cast<int>(static_cast<unsigned char>(b)) - 33;
        if (digit_value < 0 || digit_value >= ASCII85_BASE) {
            throw std::runtime_error("ASCII85: invalid character in encoded stream");
        }
        group[group_len++] = b;
        i++;
        
        // Collect remaining characters (up to 4 more)
        while (group_len < ENCODED_GROUP_SIZE && i < n) {
            // Skip whitespace
            while (i < n && is_whitespace(input[i])) {
                i++;
            }
            
            if (i >= n) break;
            
            b = input[i];
            digit_value = static_cast<int>(static_cast<unsigned char>(b)) - 33;
            if (digit_value < 0 || digit_value >= ASCII85_BASE) {
                throw std::runtime_error("ASCII85: invalid character in encoded stream");
            }
            group[group_len++] = b;
            i++;
        }
        
        // PDF 32000-1:2008 §7.4.3: final group of exactly 1 char is malformed
        if (group_len == 1) {
            throw std::runtime_error("ASCII85: partial group of length 1 is malformed");
        }
        
        // Decode the group
        int actual_digit_count;
        uint32_t value = decode_group(group, group_len, actual_digit_count);
        
        // PDF 32000-1:2008 §7.4.3: emit (group_len - 1) bytes from the 32-bit value
        int bytes_to_emit = group_len - 1;
        
        // Extract bytes from the 32-bit value (big-endian)
        for (int j = 0; j < bytes_to_emit; ++j) {
            uint32_t byte_val = (value >> (24 - j * 8)) & 0xFF;
            result.push_back(static_cast<std::byte>(byte_val));
        }
    }
    
    return result;
}

} // namespace foundation::ascii85
