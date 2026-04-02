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

#include <concepts>
#include <cstdint>
#include <limits>

namespace mpvgl {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

template <class Containing, class Contained>
concept ContainingMax = std::integral<Containing> && std::integral<Contained> &&
                        (std::numeric_limits<Containing>::max() >=
                         std::numeric_limits<Contained>::max());

template <class IntType>
    requires(ContainingMax<IntType, u8>)
static constexpr IntType U8_MAX = UINT8_MAX;

template <class IntType>
    requires(ContainingMax<IntType, u16>)
static constexpr IntType U16_MAX = UINT16_MAX;

template <class IntType>
    requires(ContainingMax<IntType, u32>)
static constexpr IntType U32_MAX = UINT32_MAX;

template <class IntType>
    requires(ContainingMax<IntType, u64>)
static constexpr IntType U64_MAX = UINT64_MAX;

template <class IntType>
    requires(ContainingMax<IntType, i8>)
static constexpr IntType I8_MAX = INT8_MAX;

template <class IntType>
    requires(ContainingMax<IntType, i16>)
static constexpr IntType I16_MAX = INT16_MAX;

template <class IntType>
    requires(ContainingMax<IntType, i32>)
static constexpr IntType I32_MAX = INT32_MAX;

template <class IntType>
    requires(ContainingMax<IntType, i64>)
static constexpr IntType I64_MAX = INT64_MAX;

template <class IntType>
static constexpr IntType ZERO = 0;

}  // namespace mpvgl
