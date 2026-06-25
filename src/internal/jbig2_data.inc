// AUTO-GENERATED — do not edit by hand.
// Source: the project spec::data_tables
// Regenerated: 2026-05-07T08:16:08.927431+00:00
// Generator: the project spec

namespace {

// T.82 §E.2.4 Table E.1 — MQ probability estimator.
// One row per state (47 states); fields are the LPS
// sub-interval boundary (qe), next state on MPS/LPS
// transitions, and the swtch (renamed from `switch`)
// flag set when MPS flips on the LPS path.

constexpr QeEntry kQeTable[47] = {
    {0x5601,  1,  1, 1},
    {0x3401,  2,  6, 0},
    {0x1801,  3,  9, 0},
    {0x0AC1,  4, 12, 0},
    {0x0521,  5, 29, 0},
    {0x0221, 38, 33, 0},
    {0x5601,  7,  6, 1},
    {0x5401,  8, 14, 0},
    {0x4801,  9, 14, 0},
    {0x3801, 10, 14, 0},
    {0x3001, 11, 17, 0},
    {0x2401, 12, 18, 0},
    {0x1C01, 13, 20, 0},
    {0x1601, 29, 21, 0},
    {0x5601, 15, 14, 1},
    {0x5401, 16, 14, 0},
    {0x5101, 17, 15, 0},
    {0x4801, 18, 16, 0},
    {0x3801, 19, 17, 0},
    {0x3401, 20, 18, 0},
    {0x3001, 21, 19, 0},
    {0x2801, 22, 19, 0},
    {0x2401, 23, 20, 0},
    {0x2201, 24, 21, 0},
    {0x1C01, 25, 22, 0},
    {0x1801, 26, 23, 0},
    {0x1601, 27, 24, 0},
    {0x1401, 28, 25, 0},
    {0x1201, 29, 26, 0},
    {0x1101, 30, 27, 0},
    {0x0AC1, 31, 28, 0},
    {0x09C1, 32, 29, 0},
    {0x08A1, 33, 30, 0},
    {0x0521, 34, 31, 0},
    {0x0441, 35, 32, 0},
    {0x02A1, 36, 33, 0},
    {0x0221, 37, 34, 0},
    {0x0141, 38, 35, 0},
    {0x0111, 39, 36, 0},
    {0x0085, 40, 37, 0},
    {0x0049, 41, 38, 0},
    {0x0025, 42, 39, 0},
    {0x0015, 43, 40, 0},
    {0x0009, 44, 41, 0},
    {0x0005, 45, 42, 0},
    {0x0001, 45, 43, 0},
    {0x5601, 46, 46, 0},
};

// T.88 §6.2.5.3 Tables 5-8 — generic-region neighbourhood
// layouts. Each kGTn array is a flat list of slots, indexed
// LSB-first (bit 0 = slot 0). A slot is fixed when adaptive
// is -1; otherwise adaptive in 0..3 indexes A1..A4 from the
// segment's GBAT array.

struct GbtSlot { int dx; int dy; int adaptive; };

constexpr GbtSlot kGT0[16] = {
    {-1,  0, -1},
    {-2,  0, -1},
    {-3,  0, -1},
    {-4,  0, -1},
    { 0,  0, 0},
    { 2, -1, -1},
    { 1, -1, -1},
    { 0, -1, -1},
    {-1, -1, -1},
    {-2, -1, -1},
    { 0,  0, 1},
    { 0,  0, 2},
    { 1, -2, -1},
    { 0, -2, -1},
    {-1, -2, -1},
    { 0,  0, 3},
};

constexpr GbtSlot kGT1[13] = {
    {-1,  0, -1},
    {-2,  0, -1},
    {-3,  0, -1},
    { 0,  0, 0},
    { 2, -1, -1},
    { 1, -1, -1},
    { 0, -1, -1},
    {-1, -1, -1},
    {-2, -1, -1},
    { 2, -2, -1},
    { 1, -2, -1},
    { 0, -2, -1},
    {-1, -2, -1},
};

constexpr GbtSlot kGT2[10] = {
    {-1,  0, -1},
    {-2,  0, -1},
    { 0,  0, 0},
    { 1, -1, -1},
    { 0, -1, -1},
    {-1, -1, -1},
    {-2, -1, -1},
    { 1, -2, -1},
    { 0, -2, -1},
    {-1, -2, -1},
};

constexpr GbtSlot kGT3[10] = {
    {-1,  0, -1},
    {-2,  0, -1},
    {-3,  0, -1},
    {-4,  0, -1},
    { 0,  0, 0},
    { 1, -1, -1},
    { 0, -1, -1},
    {-1, -1, -1},
    {-2, -1, -1},
    {-3, -1, -1},
};

struct GbtTemplate {
    int bit_count;
    int gbat_count;
    const GbtSlot* bits;
    std::int8_t default_gbat[8];   // up to 4 (x, y) pairs
    std::uint16_t sltp_ctx;
};

constexpr GbtTemplate kTemplates[4] = {
    {16, 4, kGT0, {3, -1, -3, -1, 2, -2, -2, -2}, 0x9B25},
    {13, 1, kGT1, {3, -1, 0, 0, 0, 0, 0, 0}, 0x0795},
    {10, 1, kGT2, {2, -1, 0, 0, 0, 0, 0, 0}, 0x00E5},
    {10, 1, kGT3, {2, -1, 0, 0, 0, 0, 0, 0}, 0x0195},
};

} // namespace
