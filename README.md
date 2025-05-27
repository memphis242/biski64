# biski64: Fast and Robust 2^64 Period Pseudo-Random Number Generator

This repository contains `biski64`, an extremely fast pseudo-random number generator (PRNG) with a guaranteed period of 2^64. It is designed for non-cryptographic applications where speed and statistical quality are important.

The library is available on [crates.io](https://crates.io/crates/biski64) and the documentation can be found on [docs.rs](https://docs.rs/biski64).

## Features

* **High Performance:** Significantly faster than standard library generators and competitive with or faster than other modern high-speed PRNGs like `wyrand` and `xoroshiro128++`.
* **Good Statistical Quality:** Has passed PractRand (up to 32TB) with zero anomalies and has shown exceptional results in 100 runs of BigCrush.
* **Guaranteed 2^64 Period:** Incorporates a 64-bit Weyl sequence to ensure a minimum period of 2^64.
* **Rust Ecosystem Integration:** The library is `no_std` compatible and implements the standard `RngCore` and `SeedableRng` traits from `rand_core` for easy use.

## Rust Installation

Add `biski64` to your `Cargo.toml` dependencies:

```toml
[dependencies]
biski64 = "0.1.4"
```

### Basic Usage

```rust
use biski64::Biski64Rng;
use rand_core::{RngCore, SeedableRng};

// Create a new generator from a simple 64-bit seed.
let mut rng = Biski64Rng::seed_from_u64(42);

// Generate a random u64 number.
let num = rng.next_u64();
```

## Third Party Testing

Christopher Wellons (skeeto) has tested `biski64` in his [PRNG Shootout](https://github.com/skeeto/prng64-shootout/commit/b018d283) and [commented in Reddit](https://www.reddit.com/r/C_Programming/comments/1kvhgmh/comment/muc3uvb/?context=3):

>Great stuff! When I plug it into my shootout, it's as fast as dualmix128 (i.e. saturates my benchmark), but with loopmix128's better properties. The 40-byte state is slightly heavy, but not bad at all, and certainly better than the gigantic states of classical PRNGs (Mersenne Twister, Lagged Fibonacci). **As far as I can tell, biski64 would be a good PRNG to deploy in real programs.**


## Performance

* **Rust Speed:**
```
  biski64            0.366 ns/call
  wyrand             0.428 ns/call
  xoroshiro128++     0.934 ns/call
```

* **C Speed:**
```
  biski64            0.418 ns/call
  wyrand             0.449 ns/call
  sfc64              0.451 ns/call
  xoshiro256++       0.593 ns/call
  xoroshiro128++     0.802 ns/call
  PCG128_XSL_RR_64   1.204 ns/call
```

## Rust Algorithm

```rust
use std::num::Wrapping;

// In the actual implementation, these are fields of the Biski64Rng struct.
let (mut fast_loop, mut mix, mut last_mix, mut old_rot, mut output) = 
    (Wrapping(0), Wrapping(0), Wrapping(0), Wrapping(0), Wrapping(0));

const GR: Wrapping<u64> = Wrapping(0x9e3779b97f4a7c15);

#[inline(always)]
pub fn next_u64() -> u64 {
    let old_output = output;
    let new_mix = old_rot + output;

    output = GR * mix;
    old_rot = Wrapping(last_mix.0.rotate_left(18));

    last_mix = fast_loop ^ mix;
    mix = new_mix;

    fast_loop += GR;

    old_output.0
}
```


## C Algorithm

```c
// Golden ratio fractional part * 2^64
const uint64_t GR = 0x9e3779b97f4a7c15ULL;

// Helper for rotation
static inline uint64_t rotateLeft(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

uint64_t biski64() {
  uint64_t newMix = old_rot + output;

  output = GR * mix;
  old_rot = rotateLeft(last_mix, 18);

  last_mix = fast_loop ^ mix; 
  mix = newMix;

  fast_loop += GR;

  return output;
  }
```
*(Note: See test files for full seeding and usage examples.)*


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
*(Note: For an ideal random generator, seeing three or more failures for a specific subtest would not be expected.)*




## Parallel Streams

The Weyl sequence of `biski64` is well-suited for parallel applications, and parallel streams can be implemented as follows:
* Randomly seed `mix`, `last_mix`, `old_rot` and `output` for each stream as normal.
* Assign a unique starting value to the `fast_loop` counter for each stream. To ensure maximal separation between sequences, these starting values should be spaced far apart. A simple strategy is to assign the i-th stream's counter as:
```fast_loop_i = initial_fast_loop_seed + i * GR;```

*where i is the stream index (0, 1, 2, ...) and `GR` is the golden ratio constant (0x9e3779b97f4a7c15ULL).*


## Reduced State Performance

For testing, the mixer core of `biski64` has been reduced to 64bits total state (without the Weyl sequence).  This reduced test version passes 16TB of PractRand.

```c
uint32_t output = GR * mix;
uint32_t old_rot = rotateLeft(last_mix, 11);

last_mix = GR ^ mix;
mix = old_rot + output;

return output;
```

*(Note: This a for reduced state demonstration only. Use the above full `biski64()` implementations to ensure pipelined performance and the minimum period length of 2^64.)*



## Notes
Created by Daniel Cota and named after his cat Biscuit - a small and fast Egyptian Mau.
