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

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <mio/mmap.hpp>
#include <span>
#include <variant>
#include <vector>

#include <tl/expected.hpp>

#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Error/IOError.hpp"

namespace mpvgl::io {

struct LoadPolicyMmap {};
struct LoadPolicyHeap {};
struct LoadPolicyAuto {
    std::uintmax_t threshold = 16 * 1024;
};

inline constexpr LoadPolicyMmap mmap_policy{};
inline constexpr LoadPolicyHeap heap_policy{};
inline constexpr LoadPolicyAuto auto_policy{};

class ResourceBuffer {
   public:
    using Storage = std::variant<std::vector<std::byte>, mio::mmap_source>;

    ResourceBuffer(ResourceBuffer const&) = delete;
    ResourceBuffer& operator=(ResourceBuffer const&) = delete;

    ResourceBuffer(ResourceBuffer&&) noexcept = default;
    ResourceBuffer& operator=(ResourceBuffer&&) noexcept = default;

    template <typename PolicyTag = LoadPolicyAuto>
    [[nodiscard]] static tl::expected<ResourceBuffer, Error<IOError>> load(
        std::filesystem::path const& path, PolicyTag policy = auto_policy) {
        return loadImpl(path, policy);
    }

    [[nodiscard]] std::span<std::byte const> view() const noexcept {
        return std::visit(
            [](auto const& storage) -> std::span<std::byte const> {
                using T = std::decay_t<decltype(storage)>;

                if constexpr (std::is_same_v<T, std::vector<std::byte>>) {
                    return {storage.data(), storage.size()};
                } else if constexpr (std::is_same_v<T, mio::mmap_source>) {
                    return {reinterpret_cast<std::byte const*>(storage.data()),
                            storage.size()};
                }
            },
            m_storage);
    }

   private:
    explicit ResourceBuffer(Storage storage) : m_storage(std::move(storage)) {}

    [[nodiscard]] static tl::expected<ResourceBuffer, Error<IOError>> loadImpl(
        std::filesystem::path const& path, LoadPolicyHeap) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return tl::unexpected{
                Error{IOError::FileNotFound,
                      "Failed to open file: " + path.string()}};
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<std::byte> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return tl::unexpected{
                Error{IOError::Unknown,
                      "Failed to read file to heap: " + path.string()}};
        }

        return ResourceBuffer{std::move(buffer)};
    }

    [[nodiscard]] static tl::expected<ResourceBuffer, Error<IOError>> loadImpl(
        std::filesystem::path const& path, LoadPolicyMmap) {
        std::error_code ec;
        mio::mmap_source mmap;

        mmap.map(path.string(), ec);
        if (ec) {
            return tl::unexpected{Error{
                IOError::Unknown, "Failed to mmap file: " + path.string() +
                                      " [" + ec.message() + "]"}};
        }

        return ResourceBuffer{std::move(mmap)};
    }

    [[nodiscard]] static tl::expected<ResourceBuffer, Error<IOError>> loadImpl(
        std::filesystem::path const& path, LoadPolicyAuto policy) {
        std::error_code ec;
        std::uintmax_t size = std::filesystem::file_size(path, ec);

        if (ec) {
            return tl::unexpected{
                Error{IOError::FileNotFound,
                      "Failed to get file size: " + path.string()}};
        }

        if (size >= policy.threshold) {
            return loadImpl(path, mmap_policy);
        } else {
            return loadImpl(path, heap_policy);
        }
    }

   private:
    Storage m_storage;
};

}  // namespace mpvgl::io
