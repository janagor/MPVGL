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
#include <cstdint>
#include <queue>
#include <span>
#include <vector>

#include <tl/expected.hpp>

#include "MPVGL/Error/CompressionError.hpp"
#include "MPVGL/Error/Error.hpp"

namespace mpvgl {

class HuffmanTree {
   public:
    static constexpr size_t MaxNodes = 511;
    static constexpr uint16_t NullNode = 0xFFFF;

    struct Node {
        char symbol;
        int frequency;
        uint16_t left;
        uint16_t right;

        constexpr Node() noexcept
            : symbol('\0'), frequency(0), left(NullNode), right(NullNode) {}

        constexpr Node(char s, int f, uint16_t l = NullNode,
                       uint16_t r = NullNode) noexcept
            : symbol(s), frequency(f), left(l), right(r) {}

        [[nodiscard]] constexpr bool isLeaf() const noexcept {
            return left == NullNode && right == NullNode;
        }
    };

   private:
    std::array<Node, MaxNodes> buffer{};
    uint16_t allocated_count = 0;
    uint16_t root_idx = NullNode;

    constexpr tl::expected<uint16_t, Error<CompressionError>> createNode(
        char symbol, int freq, uint16_t left = NullNode,
        uint16_t right = NullNode) {
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

        auto cmp = [this](uint16_t a, uint16_t b) {
            return buffer[a].frequency > buffer[b].frequency;
        };

        std::priority_queue<uint16_t, std::vector<uint16_t>, decltype(cmp)> pq{
            cmp};

        for (auto const& [sym, freq] : frequencies) {
            auto node_idx = createNode(sym, freq);
            if (!node_idx) return tl::unexpected{node_idx.error()};
            pq.push(node_idx.value());
        }

        while (pq.size() > 1) {
            uint16_t left = pq.top();
            pq.pop();
            uint16_t right = pq.top();
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

    [[nodiscard]] constexpr uint16_t getRootIndex() const noexcept {
        return root_idx;
    }

    [[nodiscard]] constexpr tl::expected<Node, Error<CompressionError>> getNode(
        uint16_t index) const noexcept {
        if (index >= allocated_count || index == NullNode) {
            return tl::unexpected{Error{
                CompressionError::InvalidData,
                "Attempted to access an invalid node during decompression!"}};
        }
        return buffer[index];
    }
};

}  // namespace mpvgl
