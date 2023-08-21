// Copyright (c) 2014-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPAT_ENDIAN_H
#define BITCOIN_COMPAT_ENDIAN_H

#include <bit>
#include <cstdint>

#if !defined(__cpp_lib_endian) || __cpp_lib_endian < 201907L
static_assert(false, "c++20 required");
#endif

/* Any recent and decent compiler with optimizations turned on should recognize
   the patterns here and emit clever (bswap) instructions
*/
static inline constexpr uint16_t internal_bswap_16(uint16_t x)
{
    return (x >> 8) | (x << 8);
}
static inline constexpr uint32_t internal_bswap_32(uint32_t x)
{
    return (((x & 0xff000000U) >> 24) | ((x & 0x00ff0000U) >>  8) |
            ((x & 0x0000ff00U) <<  8) | ((x & 0x000000ffU) << 24));
}
static inline constexpr uint64_t internal_bswap_64(uint64_t x)
{
     return (((x & 0xff00000000000000ull) >> 56)
          | ((x & 0x00ff000000000000ull) >> 40)
          | ((x & 0x0000ff0000000000ull) >> 24)
          | ((x & 0x000000ff00000000ull) >> 8)
          | ((x & 0x00000000ff000000ull) << 8)
          | ((x & 0x0000000000ff0000ull) << 24)
          | ((x & 0x000000000000ff00ull) << 40)
          | ((x & 0x00000000000000ffull) << 56));
}
static inline uint16_t constexpr htobe16_internal(uint16_t host_16bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_16(host_16bits);
        else return host_16bits;
}
static inline uint16_t constexpr htole16_internal(uint16_t host_16bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_16(host_16bits);
        else return host_16bits;
}
static inline uint16_t constexpr be16toh_internal(uint16_t big_endian_16bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_16(big_endian_16bits);
        else return big_endian_16bits;
}
static inline uint16_t constexpr le16toh_internal(uint16_t little_endian_16bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_16(little_endian_16bits);
        else return little_endian_16bits;
}
static inline uint32_t constexpr htobe32_internal(uint32_t host_32bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_32(host_32bits);
        else return host_32bits;
}
static inline uint32_t constexpr htole32_internal(uint32_t host_32bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_32(host_32bits);
        else return host_32bits;
}
static inline uint32_t constexpr be32toh_internal(uint32_t big_endian_32bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_32(big_endian_32bits);
        else return big_endian_32bits;
}
static inline uint32_t constexpr le32toh_internal(uint32_t little_endian_32bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_32(little_endian_32bits);
        else return little_endian_32bits;
}
static inline uint64_t constexpr htobe64_internal(uint64_t host_64bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_64(host_64bits);
        else return host_64bits;
}
static inline uint64_t constexpr htole64_internal(uint64_t host_64bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_64(host_64bits);
        else return host_64bits;
}
static inline uint64_t constexpr be64toh_internal(uint64_t big_endian_64bits)
{
    if constexpr (std::endian::native == std::endian::little) return internal_bswap_64(big_endian_64bits);
        else return big_endian_64bits;
}
static inline uint64_t constexpr le64toh_internal(uint64_t little_endian_64bits)
{
    if constexpr (std::endian::native == std::endian::big) return internal_bswap_64(little_endian_64bits);
        else return little_endian_64bits;
}

#endif // BITCOIN_COMPAT_ENDIAN_H
