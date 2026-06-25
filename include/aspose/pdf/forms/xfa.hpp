#pragma once

// =============================================================================
// Aspose::Pdf::Forms::XFA — XML Forms Architecture wrapper (PDF
// §12.7.8 — XFA Forms). Mirrors canonical Aspose.Pdf.Forms.XFA.
//
// XFA is XML-driven; most of the canonical surface returns
// System.Xml.* nodes which are dropped via translations cascade.
// v1 ships only the path-based string indexer + FieldNames vector
// — enough to query the field-name list and read string values.
// =============================================================================

#include <string>
#include <vector>

namespace Aspose::Pdf::Forms {

class XFA {
public:
    XFA() noexcept = default;

    // Path-based string accessor. Returns "" when the path
    // doesn't resolve.
    std::string operator[](const std::string& path) const;

    // Returns the list of field names in the XFA template tree.
    std::vector<std::string> FieldNames() const;

private:
    // Storage for in-memory field-path → value mappings.
    // Replaced by real XFA-template parsing when XmlDocument lands.
    std::vector<std::pair<std::string, std::string>> entries_;

    friend class Form;
};

}  // namespace Aspose::Pdf::Forms
