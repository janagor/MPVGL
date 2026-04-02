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
#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <queue>
#include <ranges>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include <tl/expected.hpp>

#include "MPVGL/Collections/Graphics/ColorSpace.hpp"
#include "MPVGL/Compression/ZigZag.hpp"
#include "MPVGL/Error/Error.hpp"
#include "MPVGL/Error/IOError.hpp"
#include "MPVGL/Graphics/Extent.hpp"
#include "MPVGL/Graphics/Indices.hpp"
#include "MPVGL/IO/JPEG/ByteStuffing.hpp"
#include "MPVGL/IO/JPEG/MCUView.hpp"
#include "MPVGL/IO/JPEG/Tables.hpp"
#include "MPVGL/Math/Transforms/IDCT.hpp"
#include "MPVGL/Utility/BitMask.hpp"
#include "MPVGL/Utility/Readers.hpp"
#include "MPVGL/Utility/Types.hpp"

namespace mpvgl::jpeg {

class JPEGLoader {
   public:
    struct Marker {
        static constexpr u16 NAM = 0xFF00, SOS = 0xFFDA, EOI = 0xFFD9,
                             SOI = 0xFFD8, DHT = 0xFFC4, DQT = 0xFFDB,
                             SOF0 = 0xFFC0;
    };

    struct TableCategory {
        static constexpr u8 DirectCurrent = 0, AlternatingCurrent = 1, SIZE = 2;
    };

   private:
    using ConstSpan = std::span<std::byte const>;
    using ConstIter = ConstSpan::iterator;

    struct DHTChunk {
        DHTChunk(JPEGLoader& loader, ConstSpan payload)
            : loader(loader), payload(payload) {}

        auto operator()() -> tl::expected<void, Error<IOError>> {
            auto iter = payload.begin();
            auto end = payload.end();

            while (iter != end) {
                if (auto parsed = parse(iter, end); !parsed) {
                    return tl::unexpected{parsed.error()};
                }
            }
            return {};
        }

       private:
        auto parse(ConstIter& iter, ConstIter const& end) const
            -> tl::expected<void, Error<IOError>> {
            if (std::distance(iter, end) < HuffmanTable::ARRAY_SIZE) {
                return tl::unexpected{Error{
                    IOError::Unknown, "DHT chunk boundary exceeded (header)"}};
            }

            u8 info = readType<u8>(iter);
            u8 category = (info >> 4) & mask::u8lsb4;
            u8 dhtId = info & mask::u8lsb4;

            if (category > 1 || dhtId > 3) {
                return tl::unexpected{
                    Error{IOError::Unknown, "Invalid DHT category or ID"}};
            }

            HuffmanTable::LengthCounts lengths{};
            lengths[0] = 0;
            std::ranges::generate(
                lengths | std::views::drop(1),
                [&iter]() -> unsigned char { return readType<u8>(iter); });

            int totalSymbols =
                std::ranges::fold_left(lengths, 0, std::plus<int>{});

            if (std::distance(iter, end) < totalSymbols) {
                return tl::unexpected{Error{
                    IOError::Unknown, "DHT chunk boundary exceeded (symbols)"}};
            }

            HuffmanTable::Symbols symbols(totalSymbols);
            std::ranges::generate(symbols, [&iter]() -> unsigned char {
                return readType<u8>(iter);
            });

            HuffmanTable table{};
            table.setup(lengths, symbols);
            loader.get().m_huffmanTables.at(category).at(dhtId) =
                std::move(table);

            return {};
        }

        ConstSpan payload;
        std::reference_wrapper<JPEGLoader> loader;
    };

    struct DQTChunk {
        DQTChunk(JPEGLoader& loader, ConstSpan payload)
            : loader(loader), payload(payload) {}

        auto operator()() -> tl::expected<void, Error<IOError>> {
            auto iter = payload.begin();
            auto end = payload.end();

            while (iter != end) {
                if (auto parsed = parse(iter, end); !parsed) {
                    return tl::unexpected{parsed.error()};
                }
            }
            return {};
        }

       private:
        auto parse(ConstIter& iter, ConstIter const& end) const
            -> tl::expected<void, Error<IOError>> {
            if (std::distance(iter, end) < 1) {
                return tl::unexpected{Error{
                    IOError::Unknown, "DQT chunk boundary exceeded (info)"}};
            }

            u8 info = readType<u8>(iter);

            constexpr u8 infoMask = 0x0F;
            auto precision = static_cast<QuantizationTable::Precision>(
                (info >> 4) & infoMask);
            u8 dqtId = info & infoMask;

            if (dqtId > 3) {
                return tl::unexpected{
                    Error{IOError::Unknown, "Invalid DQT id (must be 0-3)"}};
            }

            constexpr u8 bytesToRead8Bit = UINT8_WIDTH * 8;
            constexpr u8 bytesToRead16Bit = UINT8_WIDTH * 16;

            int bytesToRead =
                (precision == QuantizationTable::Precision::EightBit)
                    ? bytesToRead8Bit
                    : bytesToRead16Bit;

            if (std::distance(iter, end) < bytesToRead) {
                return tl::unexpected{Error{
                    IOError::Unknown, "DQT chunk boundary exceeded (data)"}};
            }

            QuantizationTable::Coefficients coeffs{};
            if (precision == QuantizationTable::Precision::EightBit) {
                std::ranges::generate(coeffs, [&iter]() -> unsigned short {
                    return readType<u8>(iter);
                });
            } else {
                std::ranges::generate(coeffs, [&iter]() -> unsigned short {
                    return readType<u16, std::endian::big>(iter);
                });
            }

            QuantizationTable table{};
            table.setup(precision, coeffs);
            loader.get().m_quantizationTables.at(dqtId) = table;

            return {};
        }

        std::reference_wrapper<JPEGLoader> loader;
        ConstSpan payload;
    };

    struct SOF0Chunk {
        SOF0Chunk(JPEGLoader& loader, ConstSpan payload)
            : loader(loader), payload(payload) {}

        auto operator()() -> tl::expected<void, Error<IOError>> {
            auto iter = payload.begin();
            auto end = payload.end();
            return parse(iter, end);
        }

       private:
        auto parse(ConstIter& iter, ConstIter const& end) const
            -> tl::expected<void, Error<IOError>> {
            constexpr std::size_t BASE_HEADER_SIZE = 6;
            if (std::distance(iter, end) < BASE_HEADER_SIZE) {
                return tl::unexpected{
                    Error{IOError::Unknown, "SOF0 chunk header too short"}};
            }

            loader.get().m_precision = readType<u8>(iter);
            loader.get().m_height = readType<u16, std::endian::big>(iter);
            loader.get().m_width = readType<u16, std::endian::big>(iter);
            std::size_t numComponents = readType<u8>(iter);

            if (std::distance(iter, end) <
                (static_cast<std::ptrdiff_t>(numComponents) * 3)) {
                return tl::unexpected{
                    Error{IOError::Unknown,
                          "SOF0 chunk size mismatch with components"}};
            }

            loader.get().m_components.resize(numComponents);

            std::ranges::generate(
                loader.get().m_components, [&iter]() -> Component {
                    Component comp{};
                    comp.id = readType<u8>(iter);
                    u8 sampling = readType<u8>(iter);
                    comp.horizontalSampling = (sampling >> 4) & mask::u8lsb4;
                    comp.verticalSampling = sampling & mask::u8lsb4;
                    comp.quantizationTableNumber = readType<u8>(iter);

                    return comp;
                });

            return {};
        }

        std::reference_wrapper<JPEGLoader> loader;
        ConstSpan payload;
    };

    struct SOSChunk {
        SOSChunk(JPEGLoader& loader, ConstSpan payload)
            : loader(loader), payload(payload) {}

        auto operator()() -> tl::expected<void, Error<IOError>> {
            auto iter = payload.begin();
            auto end = payload.end();
            return parse(iter, end);
        }

       private:
        auto parse(ConstIter& iter, ConstIter const& end) const
            -> tl::expected<void, Error<IOError>> {
            if (std::distance(iter, end) < 1) {
                return tl::unexpected{
                    Error{IOError::Unknown, "SOS chunk is too short"}};
            }

            u8 numComponentsInScan = readType<u8>(iter);

            if (std::distance(iter, end) < (numComponentsInScan * 2) + 3) {
                return tl::unexpected{
                    Error{IOError::Unknown,
                          "SOS chunk size mismatch with components"}};
            }

            for (int i = 0; i < numComponentsInScan; ++i) {
                u8 componentId = readType<u8>(iter);
                u8 tableInfo = readType<u8>(iter);

                auto compIt = std::ranges::find_if(
                    loader.get().m_components,
                    [componentId](auto const& component) -> auto {
                        return component.id == componentId;
                    });

                if (compIt != loader.get().m_components.end()) {
                    compIt->dcHuffmanTableId = (tableInfo >> 4) & mask::u8lsb4;
                    compIt->acHuffmanTableId = tableInfo & mask::u8lsb4;
                }
            }
            iter += 3;
            return {};
        }

        std::reference_wrapper<JPEGLoader> loader;
        ConstSpan payload;
    };

    using Chunk = std::variant<DHTChunk, DQTChunk, SOF0Chunk, SOSChunk>;
    using ComponentTables = std::array<HuffmanTable, 4>;
    using HuffmanTables = std::array<ComponentTables, 2>;
    using QuantizationTables = std::array<QuantizationTable, 4>;

    u32 m_width = 0;
    u32 m_height = 0;
    u8 m_precision = UINT8_WIDTH;

    bool m_endOfImage = false;

    std::vector<Component> m_components;
    HuffmanTables m_huffmanTables{};
    QuantizationTables m_quantizationTables{};
    std::queue<Chunk> m_parsingQueue;

    class Decoder {
        static constexpr size_t BLOCK_DIMENSION = 8;
        static constexpr size_t BLOCK_SIZE = BLOCK_DIMENSION * BLOCK_DIMENSION;
        static constexpr u8 MAX_COMPONENTS = 4;
        static constexpr u8 END_OF_BLOCK_SYM = 0x00;
        static constexpr u8 ZERO_RUN_LENGTH = 16;
        static constexpr std::size_t INITIAL_CODE_LENGTH = 1;
        static constexpr std::size_t AC_START_INDEX = 1;
        static constexpr int SHIFT_ONE_BIT = 1;

       public:
        explicit Decoder(JPEGLoader const& loader) : m_loader(loader) {}

        auto decode(ConstIter& iter, ConstIter end) const
            -> tl::expected<Image<PixelRGB8>, Error<IOError>> {
            ByteStuffingView stuffedView(iter, end);
            BitReader bitReader{stuffedView.begin(), stuffedView.end()};

            auto image =
                Image<PixelRGB8>{Extent2D{.width = m_loader.get().m_width,
                                          .height = m_loader.get().m_height}};
            auto view = image.view();

            std::array<i32, MAX_COMPONENTS> prevDC{};
            prevDC.fill(0);

            int maxHorizontalSampling = 0;
            int maxVerticalSampling = 0;
            for (auto const& comp : m_loader.get().m_components) {
                maxHorizontalSampling =
                    std::max(maxHorizontalSampling,
                             static_cast<int>(comp.horizontalSampling));
                maxVerticalSampling =
                    std::max(maxVerticalSampling,
                             static_cast<int>(comp.verticalSampling));
            }

            auto const mcuWidth = maxHorizontalSampling * BLOCK_DIMENSION;
            auto const mcuHeight = maxVerticalSampling * BLOCK_DIMENSION;
            auto mcusPerRow =
                (m_loader.get().m_width + mcuWidth - 1) / mcuWidth;
            auto mcusPerCol =
                (m_loader.get().m_height + mcuHeight - 1) / mcuHeight;

            std::vector<std::vector<std::array<i32, BLOCK_SIZE>>> mcuBlocks(
                m_loader.get().m_components.size());

            for (size_t compIdx = 0;
                 compIdx < m_loader.get().m_components.size(); ++compIdx) {
                auto const& comp = m_loader.get().m_components.at(compIdx);
                mcuBlocks.at(compIdx).resize(static_cast<size_t>(
                    comp.horizontalSampling * comp.verticalSampling));
            }

            for (size_t mcuRow = 0; mcuRow < mcusPerCol; ++mcuRow) {
                for (size_t mcuCol = 0; mcuCol < mcusPerRow; ++mcuCol) {
                    if (auto res = decodeMCU(bitReader, prevDC, mcuBlocks);
                        !res) {
                        return tl::unexpected{res.error()};
                    }

                    Extent2D const maxSampExtent{
                        .width = static_cast<u32>(maxHorizontalSampling),
                        .height = static_cast<u32>(maxVerticalSampling)};

                    MCUView mcuView(m_loader.get().m_components, mcuBlocks,
                                    maxSampExtent);

                    u32 const startX =
                        static_cast<u32>(mcuCol) * mcuView.width();
                    u32 const startY =
                        static_cast<u32>(mcuRow) * mcuView.height();

                    for (std::size_t i = 0; i < mcuView.size(); ++i) {
                        u32 destX =
                            startX + static_cast<u32>(i % mcuView.width());
                        u32 destY =
                            startY + static_cast<u32>(i / mcuView.width());

                        if (destX < m_loader.get().m_width &&
                            destY < m_loader.get().m_height) {
                            view({.x = destX, .y = destY}) = mcuView[i];
                        }
                    }
                }
            }

            return image;
        }

       private:
        std::reference_wrapper<JPEGLoader const> m_loader;

        template <std::input_iterator Iter, std::sentinel_for<Iter> Sentinel>
        struct BitReader {
            BitReader(Iter iter, Sentinel end)
                : iter(std::move(iter)), end(std::move(end)) {}

            auto readBits(int count) -> tl::expected<i32, Error<IOError>> {
                while (bitsLeft < count) {
                    if (iter == end) {
                        return tl::unexpected{Error{
                            IOError::EndOfFile, "EOF w strumieniu Huffmana"}};
                    }

                    u8 byte = static_cast<u8>(*iter);
                    ++iter;

                    bitBuffer = (bitBuffer << UINT8_WIDTH) | byte;
                    bitsLeft += UINT8_WIDTH;
                }

                i32 result =
                    (bitBuffer >> (bitsLeft - count)) & ((1 << count) - 1);
                bitsLeft -= count;
                return result;
            }

            auto readBit() -> tl::expected<int, Error<IOError>> {
                return readBits(1);
            }

           private:
            Iter iter;
            Sentinel end;
            u32 bitBuffer = 0;
            int bitsLeft = 0;
        };

        auto decodeMCU(auto& bitReader, std::array<i32, MAX_COMPONENTS>& prevDC,
                       std::vector<std::vector<std::array<i32, BLOCK_SIZE>>>&
                           mcuBlocks) const
            -> tl::expected<void, Error<IOError>> {
            std::array<i32, BLOCK_SIZE> dctInput{};
            std::array<i32, BLOCK_SIZE> dctOutput{};

            for (size_t compIdx = 0;
                 compIdx < m_loader.get().m_components.size(); ++compIdx) {
                auto const& comp = m_loader.get().m_components[compIdx];
                auto const& qTable = m_loader.get().m_quantizationTables.at(
                    comp.quantizationTableNumber);
                auto const& dcTable =
                    m_loader.get()
                        .m_huffmanTables.at(TableCategory::DirectCurrent)
                        .at(comp.dcHuffmanTableId);
                auto const& acTable =
                    m_loader.get()
                        .m_huffmanTables.at(TableCategory::AlternatingCurrent)
                        .at(comp.acHuffmanTableId);

                for (int vertical = 0; vertical < comp.verticalSampling;
                     ++vertical) {
                    for (int horizontal = 0;
                         horizontal < comp.horizontalSampling; ++horizontal) {
                        if (auto res =
                                decodeBlock(bitReader, dcTable, acTable, qTable,
                                            prevDC.at(compIdx), dctInput);
                            !res) {
                            return tl::unexpected{res.error()};
                        }

                        inverseDCT(dctInput, dctOutput);
                        mcuBlocks[compIdx]
                                 [(vertical * comp.horizontalSampling) +
                                  horizontal] = dctOutput;
                    }
                }
            }
            return {};
        }

        static auto decodeSymbol(auto& reader, HuffmanTable const& table)
            -> tl::expected<u8, Error<IOError>> {
            auto bitRes = reader.readBit();
            if (!bitRes) {
                return tl::unexpected{bitRes.error()};
            }

            i32 code = bitRes.value();
            std::size_t length = INITIAL_CODE_LENGTH;

            while (length <= HuffmanTable::MAX_CODE_LENGTH &&
                   code > table.maxCode(length)) {
                bitRes = reader.readBit();
                if (!bitRes) {
                    return tl::unexpected{bitRes.error()};
                }

                code = (code << SHIFT_ONE_BIT) | bitRes.value();
                length++;
            }

            if (length > HuffmanTable::MAX_CODE_LENGTH) {
                return tl::unexpected{Error{
                    IOError::Unknown, "Incorrect or broken Huffman code!"}};
            }

            std::size_t index =
                table.valOffset(length) + (code - table.minCode(length));
            return table.symbol(index);
        }

        static auto decodeBlock(auto& bitReader, HuffmanTable const& dcTable,
                                HuffmanTable const& acTable,
                                QuantizationTable const& qTable, i32& prevDC,
                                std::array<i32, BLOCK_SIZE>& outBlock)
            -> tl::expected<void, Error<IOError>> {
            outBlock.fill(0);

            auto dcCategoryRes = decodeSymbol(bitReader, dcTable);
            if (!dcCategoryRes) {
                return tl::unexpected{dcCategoryRes.error()};
            }
            u8 dcCategory = dcCategoryRes.value();

            i32 dcDiff = 0;
            if (dcCategory > 0) {
                auto bitsRes = bitReader.readBits(dcCategory);
                if (!bitsRes) {
                    return tl::unexpected{bitsRes.error()};
                }
                dcDiff = extendSign(bitsRes.value(), dcCategory);
            }

            prevDC += dcDiff;
            outBlock.at(0) = prevDC * qTable.element(0);

            int index = AC_START_INDEX;
            while (index < BLOCK_SIZE) {
                auto acSymRes = decodeSymbol(bitReader, acTable);
                if (!acSymRes) {
                    return tl::unexpected{acSymRes.error()};
                }
                u8 acSym = acSymRes.value();

                if (acSym == END_OF_BLOCK_SYM) {
                    break;
                }

                u8 numZeroes = (acSym >> 4) & mask::u8lsb4;
                u8 category = acSym & mask::u8lsb4;

                if (acSym == mask::u8msb4) {
                    index += ZERO_RUN_LENGTH;
                    continue;
                }

                index += numZeroes;

                if (index >= BLOCK_SIZE) {
                    break;
                }

                auto bitsRes = bitReader.readBits(category);
                if (!bitsRes) {
                    return tl::unexpected{bitsRes.error()};
                }
                i32 acVal = extendSign(bitsRes.value(), category);

                outBlock.at(ZIGZAG.at(index)) = acVal * qTable.element(index);
                index++;
            }

            return {};
        }
    };

   public:
    [[nodiscard]] static auto load(std::span<std::byte const> data)
        -> tl::expected<Image<PixelRGB8>, Error<IOError>> {
        auto iter = data.begin();
        auto end = data.end();

        if (std::distance(iter, end) < 2) {
            return tl::unexpected{
                Error{IOError::Unknown, "Buffer too small for SOI marker"}};
        }

        auto marker = readType<u16, std::endian::big>(iter);
        if (marker != Marker::SOI) {
            return tl::unexpected{
                Error{IOError::Unknown, "Invalid JPEG SOI marker"}};
        }

        JPEGLoader loader;
        return loader.process(iter, end);
    }

   private:
    auto process(ConstIter& iter, ConstIter end)
        -> tl::expected<Image<PixelRGB8>, Error<IOError>> {
        while (!m_endOfImage) {
            if (auto parsed = parseChunk(iter, end); !parsed) {
                return tl::unexpected{parsed.error()};
            }
            if (auto evaled = evaluate(); !evaled) {
                return tl::unexpected{evaled.error()};
            }
        };

        Decoder decoder{*this};
        return decoder.decode(iter, end);
    }

    auto parseChunk(ConstIter& iter, ConstIter end)
        -> tl::expected<void, Error<IOError>> {
        while (iter != end) {
            if (std::distance(iter, end) < 2) {
                break;
            }

            u16 marker = readType<u16, std::endian::big>(iter);

            if ((marker & mask::u16msb8) != mask::u16msb8) {
                continue;
            }

            if (marker == Marker::EOI) {
                m_endOfImage = true;
                return {};
            }

            if (std::distance(iter, end) < 2) {
                return tl::unexpected{Error{
                    IOError::EndOfFile, "Unexpected EOF before chunk length"}};
            }

            u16 length = readType<u16, std::endian::big>(iter);
            int payloadSize = length - 2;

            if (std::distance(iter, end) < payloadSize) {
                return tl::unexpected{Error{
                    IOError::EndOfFile, "Chunk payload size exceeds buffer"}};
            }

            ConstSpan payload(&*iter, payloadSize);
            iter += payloadSize;

            if (auto chunk = mapChunk(marker, payload)) {
                if (marker == Marker::SOS) {
                    m_endOfImage = true;
                }
                m_parsingQueue.push(chunk.value());
                if (marker == Marker::SOS) {
                    return {};
                }
            }
        }
        return tl::unexpected{
            Error{IOError::EndOfFile, "Unexpected EOF before SOS marker"}};
    }

    auto mapChunk(u16 marker, ConstSpan payload) -> std::optional<Chunk> {
        switch (marker) {
            case Marker::DHT:
                return DHTChunk{*this, payload};
            case Marker::DQT:
                return DQTChunk{*this, payload};
            case Marker::SOF0:
                return SOF0Chunk{*this, payload};
            case Marker::SOS:
                return SOSChunk{*this, payload};
            default:
                return std::nullopt;
        }
    }

    auto evaluate() -> tl::expected<void, Error<IOError>> {
        while (!m_parsingQueue.empty()) {
            Chunk chunk = m_parsingQueue.front();
            m_parsingQueue.pop();

            if (auto res =
                    std::visit([](auto& chk) -> auto { return chk(); }, chunk);
                !res) {
                return tl::unexpected{res.error()};
            }
        }
        return {};
    }
};

}  // namespace mpvgl::jpeg
