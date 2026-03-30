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
#include <queue>
#include <span>
#include <vector>

#include <tl/expected.hpp>

#include "MPVGL/Error/CompressionError.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl {

class HuffmanTree {
   public:
    static constexpr size_t MaxNodes = 511;
    static constexpr u16 NullNode = 0xFFFF;

    struct Node {
        char symbol;
        int frequency;
        u16 left;
        u16 right;

        constexpr Node() noexcept
            : symbol('\0'), frequency(0), left(NullNode), right(NullNode) {}

        constexpr Node(char s, int f, u16 l = NullNode,
                       u16 r = NullNode) noexcept
            : symbol(s), frequency(f), left(l), right(r) {}

        [[nodiscard]] constexpr bool isLeaf() const noexcept {
            return left == NullNode && right == NullNode;
        }
    };

   private:
    std::array<Node, MaxNodes> buffer{};
    u16 allocated_count = 0;
    u16 root_idx = NullNode;

    constexpr tl::expected<u16, Error<CompressionError>> createNode(
        char symbol, int freq, u16 left = NullNode, u16 right = NullNode) {
        if (allocated_count >= MaxNodes) {
            return tl::unexpected{
                Error{CompressionError::TreeOverflow,
                      "Maximum number of Huffman tree nodes exceeded!"}};
        }

        buffer[allocated_count] = Node(symbol, freq, left, right);
        return allocated_count++;
    }

   public:
    constexpr HuffmanTree() = default;

    constexpr tl::expected<void, Error<CompressionError>> build(
        std::span<std::pair<char, int> const> frequencies) {
        allocated_count = 0;
        root_idx = NullNode;

        if (frequencies.empty()) return {};

        auto cmp = [this](u16 a, u16 b) {
            return buffer[a].frequency > buffer[b].frequency;
        };

        std::priority_queue<u16, std::vector<u16>, decltype(cmp)> pq{cmp};

        for (auto const& [sym, freq] : frequencies) {
            auto node_idx = createNode(sym, freq);
            if (!node_idx) return tl::unexpected{node_idx.error()};
            pq.push(node_idx.value());
        }

        while (pq.size() > 1) {
            u16 left = pq.top();
            pq.pop();
            u16 right = pq.top();
            pq.pop();

            auto sum_freq = buffer[left].frequency + buffer[right].frequency;

            auto parent_idx = createNode('\0', sum_freq, left, right);
            if (!parent_idx) return tl::unexpected{parent_idx.error()};

            pq.push(parent_idx.value());
        }

        if (!pq.empty()) {
            root_idx = pq.top();
        }

        return {};
    }

    [[nodiscard]] constexpr u16 getRootIndex() const noexcept {
        return root_idx;
    }

    [[nodiscard]] constexpr tl::expected<Node, Error<CompressionError>> getNode(
        u16 index) const noexcept {
        if (index >= allocated_count || index == NullNode) {
            return tl::unexpected{Error{
                CompressionError::InvalidData,
                "Attempted to access an invalid node during decompression!"}};
        }
        return buffer[index];
    }
};

}  // namespace mpvgl
