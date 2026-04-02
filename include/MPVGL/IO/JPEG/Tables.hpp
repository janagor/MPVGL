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

#include <array>
#include <cstddef>
#include <vector>

#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::jpeg {

inline auto extendSign(i32 value, u8 category) -> i32 {
    i32 valueType = 1 << (category - 1);
    if (value < valueType) {
        value += (-1 << category) + 1;
    }
    return value;
}

class HuffmanTable {
   public:
    static constexpr std::size_t MAX_CODE_LENGTH = 16;
    static constexpr std::size_t ARRAY_SIZE = MAX_CODE_LENGTH + 1;

    using LengthCounts = std::array<u8, ARRAY_SIZE>;
    using LookupTable = std::array<i32, ARRAY_SIZE>;
    using Symbols = std::vector<u8>;

    HuffmanTable() = default;

    void setup(LengthCounts const& lengths, Symbols const& syms) {
        m_codeLengths = lengths;
        m_values = syms;
        m_isSet = true;
        generateCodes();
    }

    [[nodiscard]] auto isSet() const noexcept -> bool { return m_isSet; }
    [[nodiscard]] auto minCode(std::size_t length) const noexcept -> i32 {
        return m_minCode.at(length);
    }
    [[nodiscard]] auto maxCode(std::size_t length) const noexcept -> i32 {
        return m_maxCode.at(length);
    }
    [[nodiscard]] auto valOffset(std::size_t length) const noexcept -> i32 {
        return m_valOffset.at(length);
    }
    [[nodiscard]] auto symbol(std::size_t index) const noexcept -> u8 {
        return m_values.at(index);
    }

   private:
    LengthCounts m_codeLengths{};
    Symbols m_values;
    bool m_isSet = false;

    LookupTable m_minCode{};
    LookupTable m_maxCode{};
    LookupTable m_valOffset{};

    void generateCodes() {
        i32 code = 0;
        i32 offset = 0;
        for (std::size_t length = 1; length <= MAX_CODE_LENGTH; ++length) {
            if (m_codeLengths.at(length) == 0) {
                m_maxCode.at(length) = -1;
            } else {
                m_valOffset.at(length) = offset;
                m_minCode.at(length) = code;
                code += m_codeLengths.at(length) - 1;
                m_maxCode.at(length) = code;

                offset += m_codeLengths.at(length);
                code++;
            }
            code <<= 1;
        }
    }
};

class QuantizationTable {
   public:
    enum class Precision : u8 { EightBit = 0, SixteenBit = 1 };
    static constexpr std::size_t BLOCK_SIZE = 64;
    using Coefficients = std::array<u16, BLOCK_SIZE>;

    QuantizationTable() = default;

    void setup(Precision precision, Coefficients const& values) {
        m_precision = precision;
        m_coefficients = values;
        m_isSet = true;
    }

    [[nodiscard]] auto isSet() const noexcept -> bool { return m_isSet; }
    [[nodiscard]] auto element(std::size_t index) const noexcept -> u16 {
        return m_coefficients.at(index);
    }

   private:
    Coefficients m_coefficients{};
    Precision m_precision{Precision::EightBit};
    bool m_isSet = false;
};

}  // namespace mpvgl::jpeg
