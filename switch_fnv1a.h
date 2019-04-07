/**
 * Switch/Case strings using constexpr Fnv1-a.
 * @comment References: <https://tools.ietf.org/html/draft-eastlake-fnv-16>
 * @comment References: <https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function>
 * @maintainer xavier dot roche at algolia.com
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <string>

// Traits for FNV1a
template <size_t Bits>
struct fnv1a_traits {
    static constexpr bool Supported = false;
};

// Traits for 32-bit FNV1a
template <>
struct fnv1a_traits<32> {
    static constexpr bool Supported = true;
    using Type = uint32_t;

    static constexpr Type Prime = 0x1000193;
    static constexpr Type Offset = 0x811c9dc5;
};

// Traits for 64-bit FNV1a
template <>
struct fnv1a_traits<64> {
    static constexpr bool Supported = true;
    using Type = uint64_t;

    static constexpr Type Prime = 0x100000001b3;
    static constexpr Type Offset = 0xcbf29ce484222325;
};

static constexpr __uint128_t Pack128(uint64_t high, uint64_t low)
{
    return ((__uint128_t)high << 64) + (__uint128_t)low;
}

// Traits for 128-bit FNV1a
template <>
struct fnv1a_traits<128> {
    static constexpr bool Supported = true;
    using Type = __uint128_t;

    static constexpr Type Prime = Pack128(0x1000000, 0x000000000000013b);
    static constexpr Type Offset = Pack128(0x6c62272e07bb0142, 0x62b821756295c58d);
};

// Generic FNV1a implementation
template <size_t Bits>
struct fnv1a {
    static_assert(fnv1a_traits<Bits>::Supported);
    using Type = typename fnv1a_traits<Bits>::Type;

    /**
     * Compute the Fowler–Noll–Vo hash
     * @param s The string
     * @param l The string size
     * @return The fnv-1a hash
     */
    template <typename C>
    static constexpr Type hash(const C* s, const std::size_t l, Type hash = fnv1a_traits<Bits>::Offset)
    {
        // Accept [ unsigned | signed ] char
        static_assert(sizeof(C) == 1);

        // See <https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function>
        for (std::size_t j = 0; j < l; j++) {
            const uint8_t byte = s[j];
            hash ^= byte;
            hash *= fnv1a_traits<Bits>::Prime;
        }

        return hash;
    }

    /**
     * Compute the Fowler–Noll–Vo hash
     * @param s The string
     * @return The fnv-1a hash
     */
    template <const std::size_t l>
    static constexpr Type hash(const char (&s)[l])
    {
        return hash(&s[0], l - 1);
    }

    // Do not infer length for char arrays
    template <const std::size_t l>
    static constexpr Type hash(char (&s)[l])
    {
        return hash(&s[0]);
    }

    /**
     * Compute the Fowler–Noll–Vo hash
     * @param s The string
     * @return The fnv-1a hash
     */
    static constexpr Type hash(const char* s)
    {
        return hash(s, strlen(s));
    }

    /**
     * Compute the Fowler–Noll–Vo hash
     * @param str The string
     * @return The fnv-1a hash
     */
    static constexpr Type hash(const std::string& str)
    {
        return hash(str.c_str(), str.size());
    }
};

using fnv1a32 = fnv1a<32>;
using fnv1a64 = fnv1a<64>;
using fnv1a128 = fnv1a<128>;

constexpr auto operator"" _fnv1a32(const char* s, const std::size_t l)
{
    return fnv1a32::hash(s, l);
}
constexpr auto operator"" _fnv1a64(const char* s, const std::size_t l)
{
    return fnv1a64::hash(s, l);
}
constexpr auto operator"" _fnv1a128(const char* s, const std::size_t l)
{
    return fnv1a128::hash(s, l);
}

// Static unit tests: <https://fnvhash.github.io/fnv-calculator-online/>
static_assert("hello"_fnv1a32 == 0x4f9f2cab);
static_assert("hello"_fnv1a64 == 0xa430d84680aabd0b);
static_assert("hello"_fnv1a128 == Pack128(0xe3e1efd54283d94f, 0x7081314b599d31b3));
