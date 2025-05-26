# biski64: Fast and Robust 2^64 Period Pseudo-Random Number Generator

This repository contains `biski64`, an extremely fast pseudo-random number generator (PRNG) with a guaranteed period of 2^64. It easily passes PractRand (32TB) and BigCrush. It is designed for non-cryptographic applications where speed and statistical quality are important.

## Features

* **High Performance:** Significantly faster than standard library generators and competitive with or faster than other modern high-speed PRNGs like wyrand and xoroshiro128++.
* **Good Statistical Quality:** Has passed PractRand (512MB to 32TB) with zero anomalies. Tested in BigCrush 100 times with exceptional results (see below).
* **2^64 Period:** Minimum period length of 2^64 through its incorporated 64 bit Weyl sequence.
* **Parallel Streams:** The 64bit Weyl sequence facilitates parallel streams as outlined below.

## Performance

* **Speed:** 92% faster than C xoroshiro128++

```
  biski64            0.418 ns/call
  wyrand             0.449 ns/call
  sfc64              0.451 ns/call
  xoroshiro128++     0.802 ns/call
  PCG128_XSL_RR_64   1.204 ns/call
```


## BigCrush

BigCrush was run 100 times on `biski64` (as well as on the below mentioned reference PRNGs).

Assuming test failure with a p-value below 0.001 or above 0.999 implies a 0.2% probability of a single test yielding a "false positive" purely by chance with an ideal generator. Running BigCrush 100 times (for a total of 25400 sub-tests), around 50.8 such chance "errors" would be anticipated.

```
biski64, 47 failed subtests (out of 25400 total)
  2 subtests failed twice

wyrand, 55 failed subtests (out of 25400 total)
  1 subtest failed THREE times
  5 subtests failed twice

sfc64, 70 failed subtests (out of 25400 total)
  1 subtest failed THREE times
  12 subtests failed twice

xoroshiro128++, 54 failed subtests (out of 25400 total)
  1 subtest failed FOUR times
  4 subtests failed twice

xoroshiro256++, 60 failed subtests (out of 25400 total)
  1 subtest failed THREE times
  5 subtests failed twice

pcg128_xsl_rr_64, 47 failed subtests (out of 25400 total)
  1 subtest failed FIVE times
  4 subtests failed twice
```


## Algorithm Details

```
// Golden ratio fractional part * 2^64
const uint64_t GR = 0x9e3779b97f4a7c15ULL;

// Initialized to non-zero with SplitMix64 (or equivalent)
uint64_t fast_loop, mix, lastMix, oldRot, output; 

// Helper for rotation
static inline uint64_t rotateLeft(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

// --- biski64 ---
uint64_t biski64() {
  uint64_t newMix = oldRot + output;

  output = GR * mix;
  oldRot = rotateLeft(lastMix, 18);

  lastMix = fast_loop ^ mix; 
  mix = newMix;

  fast_loop += GR;

  return output;
  }
```

*(Note: See test files for full seeding and usage examples.)*


## Parallel Streams

The Weyl sequence of `biski64` is well-suited for parallel applications, and parallel streams can be implemented as follows:
* Randomly seed `mix`, `lastMix`, `oldRot` and `output` for each stream as normal.
* Assign a unique starting value to the `fast_loop` counter for each stream. To ensure maximal separation between sequences, these starting values should be spaced far apart. A simple strategy is to assign the i-th stream's counter as:
```fast_loop_i = initial_fast_loop_seed + i * GR;```

*where i is the stream index (0, 1, 2, ...) and `GR` is the golden ratio constant (0x9e3779b97f4a7c15ULL).*


## Reduced State Performance

For testing, the mixer core of `biski64` has been reduced to 64bits total state (without the Weyl sequence).  This reduced test version passes 16TB of PractRand.

```
uint32_t output = GR * mix;
uint32_t oldRot = rotateLeft(lastMix, 11);

lastMix = GR ^ mix;
mix = oldRot + output;

return output;
```

*(Note: This a for reduced state demonstration only. Use the above full `biski64()` implementation to ensure pipelined performance and the minimum period length of 2^64.)*


## Notes
Created by Daniel Cota and named after his cat Biscuit - a small and fast Egyptian Mau.
