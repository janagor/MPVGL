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

#include <cmath>

#include <array>
#include <cinttypes>
#include <numbers>

namespace mpvgl {

inline void inverseDCT(std::array<i32, 64> const& input,
                       std::array<i32, 64>& output) {
    std::array<f32, 64> temp{};

    f32 const m0 = 2.0f * std::cos(1.0f / 16.0f * std::numbers::pi_v<f32>);
    f32 const m1 = 2.0f * std::cos(2.0f / 16.0f * std::numbers::pi_v<f32>);
    f32 const m3 = 2.0f * std::cos(3.0f / 16.0f * std::numbers::pi_v<f32>);
    f32 const m5 = 2.0f * std::cos(5.0f / 16.0f * std::numbers::pi_v<f32>);
    f32 const s0 = 2.0f * std::cos(2.0f / 16.0f * std::numbers::pi_v<f32>);
    f32 const c4 = std::cos(4.0f / 16.0f * std::numbers::pi_v<f32>);

    auto idct1D = [&](f32* ptr) {
        f32 s00 = ptr[0], s01 = ptr[1], s02 = ptr[2], s03 = ptr[3];
        f32 s04 = ptr[4], s05 = ptr[5], s06 = ptr[6], s07 = ptr[7];

        f32 t0 = s00 * c4;
        f32 t1 = s04 * c4;
        f32 t2 = s02 * c4;
        f32 t3 = s06 * c4;

        f32 p0 = t0 + t1;
        f32 p1 = t0 - t1;
        f32 p2 = t2 + t3;
        f32 p3 = t2 - t3;

        f32 u0 = p0 + p2;
        f32 u1 = p1 + p3;
        f32 u2 = p1 - p3;
        f32 u3 = p0 - p2;

        f32 t4 = s01;
        f32 t5 = s03;
        f32 t6 = s05;
        f32 t7 = s07;

        f32 q0 = t4 + t7;
        f32 q1 = t5 + t6;
        f32 q2 = t4 - t7;
        f32 q3 = t5 - t6;

        f32 v0 = q0 + q1;
        f32 v1 = q0 - q1;
        f32 v2 = q2 + q3;
        f32 v3 = q2 - q3;

        ptr[0] = u0 + v0;
        ptr[1] = u1 + v1;
        ptr[2] = u2 + v2;
        ptr[3] = u3 + v3;
        ptr[4] = u3 - v3;
        ptr[5] = u2 - v2;
        ptr[6] = u1 - v1;
        ptr[7] = u0 - v0;
    };

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            f32 sum = 0.0f;
            for (int u = 0; u < 8; ++u) {
                f32 cu = (u == 0) ? 1.0f / std::sqrt(2.0f) : 1.0f;
                sum +=
                    cu * input[y * 8 + u] *
                    std::cos((2 * x + 1) * u * std::numbers::pi_v<f32> / 16.0f);
            }
            temp[y * 8 + x] = 0.5f * sum;
        }
    }

    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            f32 sum = 0.0f;
            for (int v = 0; v < 8; ++v) {
                f32 cv = (v == 0) ? 1.0f / std::sqrt(2.0f) : 1.0f;
                sum +=
                    cv * temp[v * 8 + x] *
                    std::cos((2 * y + 1) * v * std::numbers::pi_v<f32> / 16.0f);
            }
            output[y * 8 + x] = static_cast<i32>(std::round(0.5f * sum)) + 128;
        }
    }
}

}  // namespace mpvgl
