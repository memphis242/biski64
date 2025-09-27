#include <stdint.h> // For uint64_t and standard integer types
#include <stdio.h>  // For printf

// Unity build
#include "biski64.c"


/**
 * @brief Main function to test the biski64 PRNG.
 */
int main() {
    printf("--- biski64 Single-Threaded Test ---\n");
    biski64_state rng_state;
    uint64_t seed = 12345ULL;

    // Initialize the generator with a seed
    biski64_seed(&rng_state, seed);

    printf("Seed: %llu\n", seed);
    printf("Initial State -> fast_loop: %016llx, mix: %016llx, loop_mix: %016llx\n",
           rng_state.fast_loop, rng_state.mix, rng_state.loop_mix);

    // Generate and print a few random numbers
    printf("Generating 5 pseudo-random numbers:\n");
    for (int i = 0; i < 5; i++) {
        printf("  %d: %016llx\n", i + 1, biski64_next(&rng_state));
    }
    printf("\n");

    printf("--- biski64 Parallel Streams Test ---\n");
    biski64_state stream_state_1;
    biski64_state stream_state_2;
    uint64_t base_seed = 67890ULL;
    int total_streams = 2;

    // Initialize two separate streams from the same base seed
    biski64_stream(&stream_state_1, base_seed, 0, total_streams); // Stream 0
    biski64_stream(&stream_state_2, base_seed, 1, total_streams); // Stream 1

    printf("Base Seed: %llu, Total Streams: %d\n\n", base_seed, total_streams);

    printf("Stream 1 (Index 0) Initial State -> fast_loop: %016llx\n", stream_state_1.fast_loop);
    printf("Stream 2 (Index 1) Initial State -> fast_loop: %016llx\n\n", stream_state_2.fast_loop);

    // Generate numbers from both streams to show they produce different sequences
    printf("Generating 3 numbers from each stream:\n");
    for (int i = 0; i < 3; i++) {
        printf("  Stream 1: %016llx | Stream 2: %016llx\n",
               biski64_next(&stream_state_1),
               biski64_next(&stream_state_2));
    }

    return 0;
}
