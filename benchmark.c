#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> // For atoll
#include <time.h>   // For clock_gettime
#include <stdbool.h>

// Golden ratio constant for biski64 - golden ratio fractional part * 2^64
const uint64_t GR = 0x9e3779b97f4a7c15ULL;

// --- State Variables (Global for this benchmark) ---
// Seeded with dummy data for the benchmark

// For biski64
uint64_t fast_loop = 0xDEADBEEF12345678ULL;
uint64_t mix = 0x123456789ABCDEFULL;
uint64_t lastMix = 0x123456789ABCDEFULL;
uint64_t oldRot = 0x1112223334445556;
uint64_t output = 0x123456789ABCDEFULL;

// For wyrand
uint64_t wyrand_seed = 0xDEADBEEF12345678ULL;

// For sfc64
uint64_t sfc_a = 0xDEADBEEF12345678ULL;
uint64_t sfc_b = 0xABCDEF0123456789ULL;
uint64_t sfc_c = 0x123456789ABCDEF0ULL;
uint64_t sfc_counter = 1ULL;

// For xoroshiro128++
uint64_t xoro_s0 = 0xDEADBEEF12345678ULL;
uint64_t xoro_s1 = 0xABCDEF0123456789ULL;

// For xoroshiro256++
uint64_t xoro256_s[4] = { 0xDEADBEEF12345678ULL, 0xABCDEF0123456789ULL, 0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL };

// For PCG128_XSL_RR_64 (128-bit state, 64-bit output)
__uint128_t pcg128_state_s = (((__uint128_t)0x9ef029c7934105feULL) << 64) | 0x0bf89139a2398791ULL; // Arbitrary initial state for the 128-bit version
const __uint128_t pcg128_mult_s  = (((__uint128_t)0x2360ED051FC65DA4ULL) << 64) | 0x4385DF649FCCF645ULL; // Standard 128-bit PCG multiplier
const __uint128_t pcg128_inc_s   = (((__uint128_t)0x5851F42D4C957F2DULL) << 64) | 0x14057B7EF767814FULL;   // Standard 128-bit PCG increment (is odd)



// --- Helper Functions ---

inline uint64_t rotateLeft(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}


inline uint64_t rotateRight(const uint64_t x, int k) {
    return (x >> k) | (x << (64 - k));
}


// Get time using CLOCK_MONOTONIC for reliable interval timing
double get_time_sec() {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime failed");
        exit(EXIT_FAILURE);
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}


// --- PRNG Implementations ---


// biski64 generator function
inline uint64_t biski64() {

uint64_t newMix = oldRot + output;

output = GR * mix;
oldRot = rotateLeft(lastMix, 39);

lastMix = fast_loop ^ mix;
mix = newMix;

fast_loop += GR;

return output;
}


// Wyrand generator function (requires __uint128_t support)
inline uint64_t wyrand(void) {
    wyrand_seed += 0xa0761d6478bd642fULL;
    __uint128_t t = (__uint128_t)(wyrand_seed ^ 0xe7037ed1a0b428dbULL) * wyrand_seed;
    return (uint64_t)(t >> 64) ^ (uint64_t)t;
}


// sfc64 generator function
// Credits: Chris Doty-Humphry (PractRand)
inline uint64_t sfc64(void) {
    uint64_t tmp = sfc_a + sfc_b + sfc_counter++;
    sfc_a = sfc_b ^ (sfc_b >> 11);
    sfc_b = sfc_c + (sfc_c << 3);
    sfc_c = rotateLeft(sfc_c, 24) + tmp;
    return tmp;
}


// xoroshiro128++ generator function
inline uint64_t xoroshiro128pp(void) {
    const uint64_t s0 = xoro_s0;
    uint64_t s1 = xoro_s1;
    const uint64_t result = rotateLeft(s0 + s1, 17) + s0;

    s1 ^= s0;
    xoro_s0 = rotateLeft(s0, 49) ^ s1 ^ (s1 << 21); // a, b
    xoro_s1 = rotateLeft(s1, 28); // c
    return result;
}

// xoroshiro256++ generator function
// Credits: David Blackman and Sebastiano Vigna
inline uint64_t xoroshiro256pp(void) {
	const uint64_t result = rotateLeft(xoro256_s[0] + xoro256_s[3], 23) + xoro256_s[0];

	const uint64_t t = xoro256_s[1] << 17;

	xoro256_s[2] ^= xoro256_s[0];
	xoro256_s[3] ^= xoro256_s[1];
	xoro256_s[1] ^= xoro256_s[2];
	xoro256_s[0] ^= xoro256_s[3];

	xoro256_s[2] ^= t;

	xoro256_s[3] = rotateLeft(xoro256_s[3], 45);

	return result;
}


// PCG XSL RR 128/64 generator function (128-bit state, 64-bit output)
inline uint64_t pcg128_xsl_rr_64_random(void) {
    // LCG step for 128-bit state
    pcg128_state_s = pcg128_state_s * pcg128_mult_s + pcg128_inc_s;

    // Output function (XSL RR variant for 128-bit state, 64-bit output)
    uint64_t high_bits = (uint64_t)(pcg128_state_s >> 64);
    uint64_t low_bits  = (uint64_t)pcg128_state_s;

    uint64_t xorshifted = high_bits ^ low_bits;
    // Rotation amount is determined by the top 6 bits of the high part of the updated state
    int rotation = (int)(high_bits >> 58u);

    return rotateRight(xorshifted, rotation);
}


// --- Main Benchmark Routine ---
int main(int argc, char **argv) {
    uint64_t num_iterations = 10000000000ULL; // Default: 10 Billion iterations

    // Parse command-line argument for number of iterations
    if (argc > 1) {
        long long arg_val = atoll(argv[1]);
        if (arg_val > 0) {
            num_iterations = (uint64_t)arg_val;
        } else {
            fprintf(stderr, "Warning: Invalid number of iterations '%s', using default %llu\n",
                    argv[1], (unsigned long long)num_iterations);
        }
    }

    printf("Benchmarking PRNGs for %llu iterations...\n\n", (unsigned long long)num_iterations);

    // Use volatile to prevent compiler optimizing out the PRNG calls
    volatile uint64_t dummyVar = 0;
    double start_time, end_time, duration;
    double ns_per_call;


    // --- Benchmark biski64 ---
    printf("Benchmarking biski64...\n");

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = biski64();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  biski64 ns/call:     %.3f ns\n", ns_per_call);


    // --- Benchmark wyrand ---
    printf("\nBenchmarking wyrand...\n");

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = wyrand();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  wyrand ns/call:         %.3f ns\n", ns_per_call);


    // --- Benchmark sfc64 ---
    printf("\nBenchmarking sfc64...\n");

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = sfc64();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  sfc64 ns/call:          %.3f ns\n", ns_per_call);


    // --- Benchmark xoroshiro128++ ---
    printf("\nBenchmarking xoroshiro128++...\n");

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = xoroshiro128pp();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  xoroshiro128++ ns/call: %.3f ns\n", ns_per_call);


    // --- Benchmark xoroshiro256++ ---
    printf("\nBenchmarking xoroshiro256++...\n");

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = xoroshiro256pp();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  xoroshiro256++ ns/call: %.3f ns\n", ns_per_call);


    // --- Benchmark PCG128_XSL_RR_64 ---
    printf("\nBenchmarking PCG128_XSL_RR_64...\n");

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = pcg128_xsl_rr_64_random();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  PCG128_XSL_RR_64 ns/call: %.3f ns\n", ns_per_call);


    printf("\nBenchmark complete.\n");
    (void)dummyVar; // To prevent unused variable warning if iterations are zero.
    return 0;
}
