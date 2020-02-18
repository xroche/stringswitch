/**
 * Switch/Case strings using constexpr Fnv1-a.
 * @maintainer xavier dot roche at algolia.com
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <stdint.h>
#include <string.h>

#include "switch_fnv1a.h"

#define L_(L) #L
#define L(L) L_(L)

// Execute benchmarks ?
#define ENABLE_BENCHS

// Small (100 cases) benchs ?
// #define SMALL_BENCHS

// Do not print output for benchs ?
// #define NOPRINT_BENCHS

// Sample case
const char* dispatch(const fnv1a128::Type match)
{
    switch (match) {
    case "poney"_fnv1a128:
        return "I want one, too!";
    case "elephant"_fnv1a128:
        return "Not in my apartment please!";
    case "dog"_fnv1a128:
        return "Good puppy!";
    case "kitten"_fnv1a128:
        return "Aawwwwwwww!";
    default:
        return "Don't know this animal!";
    }
}

// Long switch of 1000 'case
constexpr const char* dispatch_1000(const fnv1a128::Type match)
{
    switch (match) {
#define WORD(W)        \
    case W##_fnv1a128: \
        return L(__LINE__)
#ifndef SMALL_BENCHS
#include "include/words-extract.h"
#else
#include "include/words-extract-small.h"
#endif
#undef WORD

    default:
        return "unknown!";
    }
}

// Hash-switch version
auto compute_hash(const char *str, size_t size)
{
    return fnv1a128::hash(str, size);
}

// Hash-switch version
const char* match_case(const std::string& str)
{
    return dispatch_1000(compute_hash(str.c_str(), str.size()));
}

// Pure-if version
const char* match_if(const std::string& str)
{
    if (str.size() == 0)
        return "";
#define WORD(W) else if (str == W) return L(__LINE__)
#ifndef SMALL_BENCHS
#include "include/words-extract.h"
#else
#include "include/words-extract-small.h"
#endif
#undef WORD
    return "unknown!";
}

static void bench(void)
{
    {
        // Small benchmarks
        constexpr size_t max = 100000;
#ifdef NOPRINT_BENCHS
        volatile size_t calc = 1;
#endif

        std::vector<std::string> strings;
#define WORD(W) strings.push_back(W)
#include "include/words-extract-match.h"
#undef WORD

        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(strings.begin(), strings.end(), std::default_random_engine(seed));

        uint64_t elapsed_if;
        uint64_t elapsed_case;

        // Naive if
        {
            auto start = std::chrono::steady_clock::now();
            for (size_t i = 0; i < max; i++) {
                for (const auto& string : strings) {
                    const char* const match = match_if(string.c_str());
#ifndef NOPRINT_BENCHS
                    std::cout << match << "\n";
#else
                    calc *= match[0];
#endif
                }
            }
            auto end = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            elapsed_if = elapsed;
            std::cerr << elapsed << "ms\n";
        }

        // Hashed switch
        {
            auto start = std::chrono::steady_clock::now();
            for (size_t i = 0; i < max; i++) {
                for (const auto& string : strings) {
                    const char* const match = match_case(string.c_str());
#ifndef NOPRINT_BENCHS
                    std::cout << match << "\n";
#else
                    calc *= match[0];
#endif
                }
            }
            auto end = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            elapsed_case = elapsed;
            std::cerr << elapsed << "ms\n";
        }

        const auto factor = (elapsed_if * 100) / elapsed_case;
        std::cerr << "Factor: " << (factor / 100) << "." << (factor % 100) << "\n";
    }
}

int main(int argc, char** argv)
{
    // Execute benchmarks ?
#ifdef ENABLE_BENCHS
    if (argc == 1)
        bench();
#endif

    // Demo
    for (int i = 1; i < argc; i++) {
        const auto hash = fnv1a128::hash(argv[i]);

        std::cout << dispatch(hash) << "\n";
        std::cout << "Hash value:" << std::hex << ((uint64_t)(hash >> 64)) << ((uint64_t)(hash)) << "\n";
    }

    return 0;
}
