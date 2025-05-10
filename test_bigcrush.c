#include "/usr/include/testu01/unif01.h"
#include "/usr/include/testu01/bbattery.h"
#include <stdint.h>
#include <stdio.h> // For printf, perror
#include <time.h>  // For clock_gettime

// Golden ratio constant for LoopMix128
const uint64_t GR = 0x9e3779b97f4a7c15ULL;

// --- State Variables (Static globals for the generator) ---
uint64_t fast_loop;
uint64_t slow_loop;
uint64_t mix;

// --- Helper Functions ---

// Rotate left function (parameter type changed to uint64_t for consistency)
uint64_t rotateLeft(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

// SplitMix64: used for seeding LoopMix128 state
// Takes a pointer to its state, updates it, and returns a generated value.
uint64_t splitmix64_next(uint64_t *sm_state) {
    uint64_t z = (*sm_state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// --- PRNG Implementation ---

// LoopMix128 generator function - returns raw 64-bit output
static double loopMix128() {

uint64_t output = GR * (mix + fast_loop);

if ( fast_loop == 0 )
  {
  slow_loop += GR;
  mix ^= slow_loop;
  }

mix = rotateLeft(mix, 59) + fast_loop;
fast_loop = rotateLeft(fast_loop, 47) + GR;

return ( output >> 11) * (1.0/9007199254740992.0);
}

// --- TestU01 Interface ---

// Main function to run TestU01 BigCrush battery
int main(void) {
    // Declare a TestU01 generator structure pointer
    unif01_Gen *gen;
    struct timespec ts;
    uint64_t time_seed;

    // --- Seed Initialization using SplitMix64 from System Time ---
    printf("Seeding LoopMix128 using SplitMix64 from clock_gettime...\n");

    // Get high-resolution time
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("clock_gettime failed");
        return 1; // Exit if cannot get time
    }

    // Combine seconds and nanoseconds into a single 64-bit seed value for SplitMix64
    // Note: Potential overflow if ts.tv_sec is very large, but unlikely in practice.
    time_seed = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;

    // Initialize LoopMix128 state using SplitMix64
    fast_loop = splitmix64_next(&time_seed);
    slow_loop = splitmix64_next(&time_seed);
    mix = splitmix64_next(&time_seed);


    printf("Seeding complete.\n");
    // --- End of Seeding ---

    // Create a TestU01 generator object from the external C function loopMix128
    // Update generator name to reflect seeding method
    printf("Creating TestU01 generator object...\n");
    gen = unif01_CreateExternGen01("LoopMix128 (time seeded)", loopMix128);
    if (gen == NULL) {
        fprintf(stderr, "Error: Failed to create TestU01 generator.\n");
        return 1; // Exit if generator creation failed
    }

    // Run the BigCrush test battery on the generator
    printf("Starting TestU01 BigCrush for LoopMix128...\n");
    bbattery_BigCrush(gen);
    printf("TestU01 BigCrush finished.\n");

    // Clean up: delete the generator object to free resources
    unif01_DeleteExternGen01(gen);

    // Return success
    return 0;
}
