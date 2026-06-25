#pragma once

// =============================================================================
// Aspose::Pdf::Facades::ISaveableFacade — extension of IFacade
// for facades that mutate the bound document and serialise the
// result back through Save. Mirrors canonical
// Aspose.Pdf.Facades.ISaveableFacade.
//
// Phased drops:
//   * Save(Stream) — Stream cascade (auto-drop)
// =============================================================================

#include <string>

#include <aspose/pdf/facades/i_facade.hpp>

namespace Aspose::Pdf::Facades {

class ISaveableFacade : public virtual IFacade {
public:
    ~ISaveableFacade() override = default;

    virtual void Save(const std::string& destFile) = 0;
};

}  // namespace Aspose::Pdf::Facades
