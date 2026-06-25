#pragma once

// =============================================================================
// Aspose::Pdf::XmpValue — discriminated value carried in Metadata.
//
// v1 covers the four scalar shapes a PDF /Metadata stream actually
// exercises: string, integer, double, and ordered XmpValue arrays.
// The richer canonical shapes (DateTime, XmpField, raw XmlNode,
// KeyValuePair-backed "named values", Dictionary, structures) are
// out of v1 scope — their Is* predicates always return false.
//
// The DateTime ctor + ToDateTime accessor are dropped from the cpp
// surface entirely; the corresponding canonical members aren't in
// the catalog yaml. See capabilities/xmp_value.yaml for the
// language-neutral contract and the per-target BCL substitutions.
// =============================================================================

#include <string>
#include <vector>

namespace Aspose::Pdf {

class XmpValue {
public:
    explicit XmpValue(const std::string& value);
    explicit XmpValue(int value);
    explicit XmpValue(double value);
    explicit XmpValue(std::vector<XmpValue> array);

    XmpValue(const XmpValue&) = default;
    XmpValue& operator=(const XmpValue&) = default;
    XmpValue(XmpValue&&) noexcept = default;
    XmpValue& operator=(XmpValue&&) noexcept = default;
    ~XmpValue() = default;

    std::string ToStringValue() const;
    int ToInteger() const;
    double ToDouble() const;
    std::vector<XmpValue> ToArray() const;
    std::string ToString() const;

    bool IsString() const noexcept;
    bool IsInteger() const noexcept;
    bool IsDouble() const noexcept;
    bool IsArray() const noexcept;

    bool IsDateTime() const noexcept    { return false; }
    bool IsField() const noexcept       { return false; }
    bool IsNamedValue() const noexcept  { return false; }
    bool IsRaw() const noexcept         { return false; }
    bool IsNamedValues() const noexcept { return false; }
    bool IsStructure() const noexcept   { return false; }

private:
    enum class Kind { String, Integer, Double, Array };

    Kind kind_;
    std::string str_;
    int int_ = 0;
    double double_ = 0.0;
    std::vector<XmpValue> array_;
};

}
