#include <aspose/pdf/xmp_value.hpp>

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Aspose::Pdf {

XmpValue::XmpValue(const std::string& value)
    : kind_(Kind::String), str_(value) {}

XmpValue::XmpValue(int value)
    : kind_(Kind::Integer), int_(value) {}

XmpValue::XmpValue(double value)
    : kind_(Kind::Double), double_(value) {}

XmpValue::XmpValue(std::vector<XmpValue> array)
    : kind_(Kind::Array), array_(std::move(array)) {}

std::string XmpValue::ToStringValue() const {
    switch (kind_) {
    case Kind::String:
        return str_;
    case Kind::Integer:
        return std::to_string(int_);
    case Kind::Double: {
        std::ostringstream oss;
        oss.imbue(std::locale::classic());
        oss.precision(17);
        oss << double_;
        return oss.str();
    }
    case Kind::Array: {
        std::string out;
        for (std::size_t i = 0; i < array_.size(); ++i) {
            if (i > 0) out.push_back('\t');
            out += array_[i].ToStringValue();
        }
        return out;
    }
    }
    throw std::runtime_error("XmpValue: corrupt kind");
}

int XmpValue::ToInteger() const {
    switch (kind_) {
    case Kind::Integer: return int_;
    case Kind::Double:  return static_cast<int>(double_);
    case Kind::String: {
        try { return std::stoi(str_); }
        catch (const std::exception&) {
            throw std::runtime_error(
                "XmpValue: not convertible to integer");
        }
    }
    default:
        throw std::runtime_error(
            "XmpValue: not convertible to integer");
    }
}

double XmpValue::ToDouble() const {
    switch (kind_) {
    case Kind::Double:  return double_;
    case Kind::Integer: return static_cast<double>(int_);
    case Kind::String: {
        try { return std::stod(str_); }
        catch (const std::exception&) {
            throw std::runtime_error(
                "XmpValue: not convertible to double");
        }
    }
    default:
        throw std::runtime_error(
            "XmpValue: not convertible to double");
    }
}

std::vector<XmpValue> XmpValue::ToArray() const {
    if (kind_ != Kind::Array)
        throw std::runtime_error("XmpValue: not an array");
    return array_;
}

std::string XmpValue::ToString() const { return ToStringValue(); }

bool XmpValue::IsString() const noexcept  { return kind_ == Kind::String; }
bool XmpValue::IsInteger() const noexcept { return kind_ == Kind::Integer; }
bool XmpValue::IsDouble() const noexcept  { return kind_ == Kind::Double; }
bool XmpValue::IsArray() const noexcept   { return kind_ == Kind::Array; }

}
