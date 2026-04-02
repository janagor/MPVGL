/**
 *  Your Project
 *
 *  Copyright (C) 2026 Jan Aleksander Górski
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would
 *     be appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *
 *  3. This notice may not be removed or altered from any source
 *     distribution.
 */
#pragma once

#include <algorithm>

#include "MPVGL/Collections/Graphics/ColorSpace.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

template <class PixelFrom, class PixelTo>
auto convert(PixelFrom const& /*pixelFrom*/, PixelTo& /*pixelTo*/) -> void;

template <>
inline auto convert<PixelYCbCr8, PixelRGB8>(PixelYCbCr8 const& pixelFrom,
                                            PixelRGB8& pixelTo) -> void {
    using Color = PixelYCbCr8;
    constexpr int CHROMA_OFFSET = 128;

    constexpr f32 COEFF_R_CR = 1.402F;
    constexpr f32 COEFF_G_CB = 0.344136F;
    constexpr f32 COEFF_G_CR = 0.714136F;
    constexpr f32 COEFF_B_CB = 1.772F;

    constexpr int MIN_RGB_VAL = 0;
    constexpr int MAX_RGB_VAL = 255;

    f32 const luminance = static_cast<f32>(pixelFrom.y());
    f32 const chromaBlue = static_cast<f32>(pixelFrom.cb() - CHROMA_OFFSET);
    f32 const chromaRed = static_cast<f32>(pixelFrom.cr() - CHROMA_OFFSET);

    int const red = static_cast<int>(luminance + (COEFF_R_CR * chromaRed));
    int const green = static_cast<int>(luminance - (COEFF_G_CB * chromaBlue) -
                                       (COEFF_G_CR * chromaRed));
    int const blue = static_cast<int>(luminance + (COEFF_B_CB * chromaBlue));
    pixelTo.r() = static_cast<u8>(std::clamp(red, MIN_RGB_VAL, MAX_RGB_VAL));
    pixelTo.g() = static_cast<u8>(std::clamp(green, MIN_RGB_VAL, MAX_RGB_VAL));
    pixelTo.b() = static_cast<u8>(std::clamp(blue, MIN_RGB_VAL, MAX_RGB_VAL));
}

}  // namespace mpvgl
