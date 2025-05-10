#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> // For atoll
#include <time.h>   // For clock_gettime
#include <stdbool.h>

// Golden ratio constant for LoopMix128 - golden ratio fractional part * 2^64
const uint64_t GR = 0x9e3779b97f4a7c15ULL;

// --- State Variables (Global for this benchmark) ---
// Seeded with dummy data for the benchmark

// For LoopMix128
uint64_t fast_loop = 0xDEADBEEF12345678ULL;
uint64_t slow_loop = 0xABCDEF0123456789ULL;
uint64_t mix = 0x123456789ABCDEFULL;

// For xoroshiro128++
uint64_t xoro_s0 = 0xDEADBEEF12345678ULL;
uint64_t xoro_s1 = 0xABCDEF0123456789ULL;

// For wyrand
uint64_t wyrand_seed = 0xDEADBEEF12345678ULL;

// For PCG64
uint64_t pcg_state = 0x853c49e6748fea9bULL; // Standard PCG initial state
uint64_t pcg_inc = 0xda3e39cb94b95bdbULL;   // Standard PCG increment (must be odd)


// --- Helper Functions ---

// Rotate left function (used by LoopMix and xoroshiro)
inline uint64_t rotateLeft(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

// Rotate right function (used by PCG64)
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


// LoopMix128 generator function
inline uint64_t loopMix128() {
    uint64_t output = GR * (mix + fast_loop);

    if ( fast_loop == 0 )
      {
      slow_loop += GR;
      mix ^= slow_loop;
      }

    mix = rotateLeft(mix, 59) + fast_loop;
    fast_loop = rotateLeft(fast_loop, 47) + GR;

    return output;
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


// Wyrand generator function (requires __uint128_t support)
inline uint64_t wyrand(void) {
    wyrand_seed += 0xa0761d6478bd642fULL;
    __uint128_t t = (__uint128_t)(wyrand_seed ^ 0xe7037ed1a0b428dbULL) * wyrand_seed;
    return (uint64_t)(t >> 64) ^ (uint64_t)t;
}

// PCG64 (PCG XSL RR 64/64) generator function
inline uint64_t pcg64_random(void) {
    uint64_t oldstate = pcg_state;
    pcg_state = oldstate * 6364136223846793005ULL + (pcg_inc | 1); // LCG step
    // Output function (PCG XSL RR):
    uint64_t xsh = ((oldstate >> 22u) ^ oldstate);
    return rotateRight(xsh, (oldstate >> 61u) + 22u);
}


// --- Main Benchmark Routine ---
int main(int argc, char **argv) {
    uint64_t num_iterations = 10000000000ULL; // Default: 10 Billion iterations
    uint64_t warmup_iterations = 1000000000ULL; // Default: 1 Billion iterations

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


    // --- Benchmark LoopMix128 ---
    printf("Benchmarking LoopMix128...\n");

    // warmup
    for (uint64_t i = 0; i < warmup_iterations; ++i)
        dummyVar = loopMix128();

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = loopMix128();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  LoopMix128 ns/call:     %.3f ns\n", ns_per_call);

    // --- Benchmark xoroshiro128++ ---
    printf("\nBenchmarking xoroshiro128++...\n");

    // warmup
    for (uint64_t i = 0; i < warmup_iterations; ++i)
        dummyVar = xoroshiro128pp();

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = xoroshiro128pp();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  xoroshiro128++ ns/call: %.3f ns\n", ns_per_call);

    // --- Benchmark wyrand ---
    printf("\nBenchmarking wyrand...\n");

    // warmup
    for (uint64_t i = 0; i < warmup_iterations; ++i)
        dummyVar = wyrand();

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = wyrand();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  wyrand ns/call:         %.3f ns\n", ns_per_call);

    // --- Benchmark PCG64 ---
    printf("\nBenchmarking PCG64...\n");

    // warmup
    for (uint64_t i = 0; i < warmup_iterations; ++i)
        dummyVar = pcg64_random();

    // For an even playing field make sure that all benchmarking loops are equivalently aligned
    asm volatile (".balign 16");

    start_time = get_time_sec();
    for (uint64_t i = 0; i < num_iterations; ++i)
        dummyVar = pcg64_random();

    end_time = get_time_sec();
    duration = end_time - start_time;
    ns_per_call = (duration * 1e9) / num_iterations;
    printf("  PCG64 ns/call:          %.3f ns\n", ns_per_call);


    printf("\nBenchmark complete.\n");
    (void)dummyVar; // To prevent unused variable warning if iterations are zero.
    return 0;
}
