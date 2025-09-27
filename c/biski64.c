#include <stdint.h> // For uint64_t and standard integer types
#include <stdio.h>  // For printf


/**
 * @brief State structure for the biski64 PRNG.
 *
 * Holds the internal state variables required by the biski64 algorithm.
 * This structure should be initialized via biski64_seed() or biski64_stream().
 */
typedef struct {
    uint64_t fast_loop;
    uint64_t mix;
    uint64_t loop_mix;
} biski64_state;

uint64_t biski64_next(biski64_state* state);


/**
 * @internal
 * @brief Advances a 64-bit SplitMix64 PRNG state and returns a pseudo-random number.
 *
 * This is used internally by seeding functions to expand a single 64-bit seed
 * into the full biski64_state.
 *
 * @param seed_state_ptr Pointer to the 64-bit state of the SplitMix64 generator.
 * This state is advanced by the function. It is the caller's responsibility
 * to ensure this pointer is not NULL.
 * @return A 64-bit pseudo-random unsigned integer.
 */
static uint64_t splitmix64_next(uint64_t* seed_state_ptr) {
    uint64_t z = (*seed_state_ptr += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}


/**
 * @brief A private helper to warm up the generator by cycling it several times.
 *
 * This function should be called after seeding to discard the initial states,
 * which might have some statistical weaknesses. The `static` keyword limits
 * its scope to this file, making it a private helper.
 *
 * @param state Pointer to the biski64_state structure to be warmed up.
 */
static void biski64_warmup(biski64_state* state) {
    for (int i = 0; i < 16; ++i) {
        biski64_next(state); // Assumes this function advances the state
    }
}


/**
 * @brief Initializes the state of a biski64 PRNG instance from a single 64-bit seed.
 *
 * Uses a SplitMix64 generator to derive the initial values for all internal
 * state variables (`fast_loop`, `mix`, `loop_mix`) from the provided seed.
 * This ensures that different seeds produce well-distributed initial states.
 * Suitable for single-threaded use or when parallel stream spacing is not required.
 *
 * @param state Pointer to the biski64_state structure to be initialized.
 * The caller must ensure this pointer is not NULL.
 * @param seed  The 64-bit value to use as the seed.
 */
void biski64_seed(biski64_state* state, uint64_t seed) {
    // It is the caller's responsibility to ensure 'state' is not NULL.
    uint64_t seeder_state = seed;

    // Derive initial values for each biski64 state variable.
    state->mix       = splitmix64_next(&seeder_state);
    state->loop_mix  = splitmix64_next(&seeder_state);
    state->fast_loop = splitmix64_next(&seeder_state);

    biski64_warmup(state);
}


/**
 * @brief Initializes the state of a biski64 PRNG stream when using parallel streams.
 *
 * Initializes `mix` and `loop_mix` from the provided `seed` using SplitMix64.
 * Initializes `fast_loop` based on `streamIndex` and `totalNumStreams` to ensure
 * distinct, well-spaced sequences for parallel execution.
 *
 * @param state Pointer to the biski64_state structure to be initialized.
 * The caller must ensure this pointer is not NULL.
 * @param seed The base 64-bit value to use for seeding `mix` and `loop_mix`.
 * @param streamIndex The index of the current stream (0 to totalNumStreams-1).
 * The caller must ensure 0 <= streamIndex < totalNumStreams.
 * @param totalNumStreams The total number of streams.
 * The caller must ensure this is >= 1.
 */
void biski64_stream(biski64_state* state, uint64_t seed, int streamIndex, int totalNumStreams) {
    // It is the caller's responsibility to ensure 'state' is not NULL,
    // totalNumStreams >= 1, and 0 <= streamIndex < totalNumStreams.

    uint64_t seeder_state = seed;

    state->mix      = splitmix64_next(&seeder_state);
    state->loop_mix = splitmix64_next(&seeder_state);

    if (totalNumStreams == 1)
        state->fast_loop = splitmix64_next(&seeder_state);
    else {
        // Space out fast_loop starting values for parallel streams.
        uint64_t cyclesPerStream = ((uint64_t)-1) / ((uint64_t)totalNumStreams);
        state->fast_loop = (uint64_t) streamIndex * cyclesPerStream * 0x9999999999999999ULL;
    }

    biski64_warmup(state);
}


/**
 * @internal
 * @brief Performs a 64-bit left rotation.
 *
 * @param x The value to rotate.
 * @param k The number of bits to rotate by. Must be in the range [0, 63].
 * @return The result of rotating x left by k bits.
 */
static inline uint64_t rotate_left(const uint64_t x, int k) {
    // Assuming k is within valid range [0, 63] as per function contract.
    return (x << k) | (x >> (-k & 63));
}


/**
 * @brief Generates the next 64-bit pseudo-random number from a biski64 PRNG instance.
 *
 * Advances the PRNG state and returns a new pseudo-random number.
 *
 * @param state Pointer to the biski64_state structure. Must have been initialized
 * by a seeding function. The caller must ensure this pointer is not NULL.
 * @return A 64-bit pseudo-random unsigned integer.
 */
uint64_t biski64_next(biski64_state* state) {
    // It is the caller's responsibility to ensure 'state' is not NULL and initialized.

    const uint64_t output = state->mix + state->loop_mix;
    const uint64_t old_loop_mix = state->loop_mix;

    state->loop_mix = state->fast_loop ^ state->mix;
    state->mix = rotate_left(state->mix, 16) +
                 rotate_left(old_loop_mix, 40);
    state->fast_loop += 0x9999999999999999ULL; // Additive constant for the Weyl sequence.

    return output;
}


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
