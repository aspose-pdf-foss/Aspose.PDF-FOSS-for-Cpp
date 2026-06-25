#include "palette_quantiser.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace foundation::palette_quantiser {

namespace {

// One pixel as three component bytes — keep tightly packed so
// std::sort over millions of pixels stays cache-friendly.
struct Pixel {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

// One bucket = a contiguous slice of ``pixels`` (begin..end).
// Tracks min/max along each channel so we can pick the splitting
// channel by largest extent.
struct Bucket {
    std::size_t begin;
    std::size_t end;       // exclusive
    std::uint8_t min_r, max_r;
    std::uint8_t min_g, max_g;
    std::uint8_t min_b, max_b;

    std::size_t size() const noexcept { return end - begin; }
    int extent_r() const noexcept { return max_r - min_r; }
    int extent_g() const noexcept { return max_g - min_g; }
    int extent_b() const noexcept { return max_b - min_b; }
    int max_extent() const noexcept {
        return std::max({extent_r(), extent_g(), extent_b()});
    }
};

void ComputeBounds(Bucket& b, const std::vector<Pixel>& pixels) {
    b.min_r = b.min_g = b.min_b = 255;
    b.max_r = b.max_g = b.max_b = 0;
    for (std::size_t i = b.begin; i < b.end; ++i) {
        const auto& p = pixels[i];
        b.min_r = std::min(b.min_r, p.r);
        b.max_r = std::max(b.max_r, p.r);
        b.min_g = std::min(b.min_g, p.g);
        b.max_g = std::max(b.max_g, p.g);
        b.min_b = std::min(b.min_b, p.b);
        b.max_b = std::max(b.max_b, p.b);
    }
}

std::array<std::uint8_t, 3> Mean(const Bucket& b,
                                 const std::vector<Pixel>& pixels) {
    std::uint64_t sr = 0, sg = 0, sb = 0;
    for (std::size_t i = b.begin; i < b.end; ++i) {
        sr += pixels[i].r;
        sg += pixels[i].g;
        sb += pixels[i].b;
    }
    const auto n = static_cast<std::uint64_t>(b.size());
    return {static_cast<std::uint8_t>(sr / n),
            static_cast<std::uint8_t>(sg / n),
            static_cast<std::uint8_t>(sb / n)};
}

// Squared distance between an RGB triple and a palette entry.
std::uint32_t Dist2(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                    const std::array<std::uint8_t, 3>& pal) {
    const int dr = static_cast<int>(r) - pal[0];
    const int dg = static_cast<int>(g) - pal[1];
    const int db = static_cast<int>(b) - pal[2];
    return static_cast<std::uint32_t>(dr * dr + dg * dg + db * db);
}

}  // namespace

IndexedImage Quantise(std::span<const std::uint8_t> rgba,
                      std::uint32_t width,
                      std::uint32_t height,
                      std::uint32_t max_colors) {
    if (max_colors < 2 || max_colors > 256) {
        throw std::invalid_argument(
            "palette_quantiser: max_colors must be in [2, 256]");
    }
    const std::size_t n_pixels = static_cast<std::size_t>(width) * height;
    if (rgba.size() < n_pixels * 4) {
        throw std::invalid_argument(
            "palette_quantiser: rgba buffer smaller than width*height*4");
    }

    // Drop alpha; copy R/G/B into a working buffer that median-cut
    // will sort in place.
    std::vector<Pixel> pixels(n_pixels);
    for (std::size_t i = 0; i < n_pixels; ++i) {
        pixels[i] = {rgba[i * 4 + 0], rgba[i * 4 + 1], rgba[i * 4 + 2]};
    }

    // Initial bucket spans the whole pixel array.
    std::vector<Bucket> buckets;
    buckets.reserve(max_colors);
    Bucket root{0, n_pixels, 0, 0, 0, 0, 0, 0};
    ComputeBounds(root, pixels);
    buckets.push_back(root);

    // Median-cut: while we have fewer than max_colors buckets,
    // pick the one with the largest extent on any channel and
    // split it at the median value of that channel.
    while (buckets.size() < max_colors) {
        // Find the bucket with the largest extent across any
        // channel. Ties broken by index order (deterministic).
        std::size_t best = 0;
        int best_extent = -1;
        for (std::size_t i = 0; i < buckets.size(); ++i) {
            if (buckets[i].size() < 2) continue;
            const int e = buckets[i].max_extent();
            if (e > best_extent) {
                best = i;
                best_extent = e;
            }
        }
        if (best_extent <= 0) break;  // no further splits possible

        Bucket& b = buckets[best];
        const int er = b.extent_r();
        const int eg = b.extent_g();
        const int eb = b.extent_b();

        // Sort the bucket's pixel slice along the dominant axis.
        auto begin_it = pixels.begin() + b.begin;
        auto end_it   = pixels.begin() + b.end;
        if (er >= eg && er >= eb) {
            std::sort(begin_it, end_it,
                [](const Pixel& a, const Pixel& c) { return a.r < c.r; });
        } else if (eg >= er && eg >= eb) {
            std::sort(begin_it, end_it,
                [](const Pixel& a, const Pixel& c) { return a.g < c.g; });
        } else {
            std::sort(begin_it, end_it,
                [](const Pixel& a, const Pixel& c) { return a.b < c.b; });
        }
        const std::size_t mid = b.begin + b.size() / 2;

        Bucket left {b.begin, mid, 0, 0, 0, 0, 0, 0};
        Bucket right{mid,     b.end, 0, 0, 0, 0, 0, 0};
        ComputeBounds(left,  pixels);
        ComputeBounds(right, pixels);

        buckets[best] = left;
        buckets.push_back(right);
    }

    // Build the palette as the bucket means.
    IndexedImage out;
    out.width  = width;
    out.height = height;
    out.palette.reserve(buckets.size());
    for (const auto& bucket : buckets) {
        out.palette.push_back(Mean(bucket, pixels));
    }

    // Map every original input pixel to the palette entry it's
    // closest to (squared Euclidean distance in RGB). Linear
    // search over the palette is fine — palette is at most 256.
    out.indices.resize(n_pixels);
    for (std::size_t i = 0; i < n_pixels; ++i) {
        const std::uint8_t r = rgba[i * 4 + 0];
        const std::uint8_t g = rgba[i * 4 + 1];
        const std::uint8_t b = rgba[i * 4 + 2];
        std::uint8_t best_idx = 0;
        std::uint32_t best_d = Dist2(r, g, b, out.palette[0]);
        for (std::size_t k = 1; k < out.palette.size(); ++k) {
            const auto d = Dist2(r, g, b, out.palette[k]);
            if (d < best_d) { best_d = d; best_idx = static_cast<std::uint8_t>(k); }
        }
        out.indices[i] = best_idx;
    }
    return out;
}

}  // namespace foundation::palette_quantiser
