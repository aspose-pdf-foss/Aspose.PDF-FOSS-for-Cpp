#pragma once

// =============================================================================
// Aspose::Pdf::Devices::Device — abstract root of the device class
// hierarchy. No members in the canonical assembly; exists as the
// common base type for PageDevice (and, transitively, DocumentDevice
// + ImageDevice).
// =============================================================================

namespace Aspose::Pdf::Devices {

class Device {
public:
    virtual ~Device() = default;

protected:
    Device() = default;
};

}
