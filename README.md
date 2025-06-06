# biski64: Fast and Robust Pseudo-Random Number Generator

This repository contains `biski64`, an extremely fast pseudo-random number generator (PRNG) with a guaranteed minimum period of 2^64. It is designed for non-cryptographic applications where speed and statistical quality are important.

The library is available on [crates.io](https://crates.io/crates/biski64) and the documentation can be found on [docs.rs](https://docs.rs/biski64).

## Features

* **High Performance:** - Significantly faster than standard library generators and modern high-speed PRNGs like `xoroshiro128++` and `xoshiro256++`.
* **Exceptional Statistical Quality:** - Easily passes BigCrush and terabytes of PractRand. [Scaled down versions](#scaled-down-testing) show even better mixing efficiency than well respected PRNGs like JSF.
* **Guaranteed Minimum Period:** - Incorporates a 64-bit Weyl sequence to ensure a minimum period of 2^64.
* **Proven Injectivity:** - Invertible algorithm with proven injectivity via Z3 Prover.
* **Rust Ecosystem Integration:** - The library is `no_std` compatible and implements the standard `RngCore` and `SeedableRng` traits from `rand_core` for easy use.
* **C Integration:** - Only requires stdint.h.


## Rust Installation

Add `biski64` and `rand` to your `Cargo.toml` dependencies:

```toml
[dependencies]
biski64 = "0.3.0"
rand = "0.9"
```

### Basic Usage

```rust
use rand::{RngCore, SeedableRng};
use biski64::Biski64Rng;

let mut rng = Biski64Rng::seed_from_u64(12345);
let num = rng.next_u64();
```

## Performance

* **Rust:**
```
  biski64            0.401 ns/call
  xoshiro256++       0.610 ns/call
  xoroshiro128++     0.883 ns/call
```

* **C:**
```
  biski64            0.368 ns/call
  wyrand             0.368 ns/call
  sfc64              0.368 ns/call
  xoshiro256++       0.552 ns/call
  xoroshiro128++     0.732 ns/call
  PCG128_XSL_RR_64   1.197 ns/call
```

* **Java:**
```
  biski64            0.491 ns/call
  xoshiro256++       0.739 ns/call
  xoroshiro128++     0.790 ns/call
  ThreadLocalRandom  0.846 ns/call
  Java.util.Random   5.315 ns/call
```
*Tested on an AMD Ryzen 9 7950X3D*


## Rust Algorithm

```rust
use std::num::Wrapping;

// In the actual implementation, these are fields of the Biski64Rng struct.
let (mut fast_loop, mut mix, mut loop_mix) = 
    (Wrapping(0), Wrapping(0), Wrapping(0));

#[inline(always)]
pub fn next_u64(&mut self) -> u64 {
  let output = self.mix + self.loop_mix;
  let old_loop_mix = self.loop_mix;

  self.loop_mix = self.fast_loop ^ self.mix;
  self.mix = Wrapping(self.mix.0.rotate_left(16)) + Wrapping(old_loop_mix.0.rotate_left(40));

  self.fast_loop += 0x9999999999999999;

  output.0
  }
```


## C Algorithm

```c
// Helper for rotation
static inline uint64_t rotateLeft(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

uint64_t biski64() {
  uint64_t output = mix + loopMix;
  uint64_t oldLoopMix = loopMix;

  loopMix = fast_loop ^ mix;
  mix = rotateLeft(mix, 16) + rotateLeft(oldLoopMix, 40);

  fast_loop += 0x9999999999999999;

  return output;
  }
```


## Java Algorithm

```java
public long nextLong() {
  final long output = mix + loopMix;
  final long oldLoopMix = loopMix;

  loopMix = fastLoop ^ mix;
  mix = Long.rotateLeft( mix, 16 ) + Long.rotateLeft( oldLoopMix, 40 );
  fastLoop += 0x9999999999999999L;

  return output;
  }
```


*(Note: See test files for full seeding and usage examples.)*


## Parallel Streams

`biski64` is well-suited for parallel applications, and parallel streams can be implemented as follows:
* Randomly seed `mix`, and `loop_mix` for each stream as normal.
* To ensure maximal separation between sequences, space starting values for each streams' `fast_loop` using something like:
```
uint64_t cycles_per_stream = (2^64 - 1 ) / numStreams;
fast_loop = i * cycles_per_stream * 0x9999999999999999ULL;
```
*where i is the stream index (0, 1, 2, ...)*


## Scaled Down Testing

A key test for any random number generator is to see how it performs when its internal state is drastically reduced. This allows for practical testing of the core mixing algorithm.  `biski64` performs exceptionally well in this regard.

| State Variable Size  | Total State | Practrand Failure |
| ------------- | ------------- | ------------- |
| 8-bit  | 24 bits | [2^22 bytes (4 MB)](https://github.com/danielcota/biski64_dev/blob/main/c_reference/test_practrand_8bit_out.txt)|
| 16-bit  | 48 bits  | [2^40 bytes (1 TB)](https://github.com/danielcota/biski64_dev/blob/main/c_reference/test_practrand_16bit_out.txt) |
| 32-bit  | 96 bits  | [did not fail, tested to 2^46 bytes (64 TB)](https://github.com/danielcota/biski64_dev/blob/main/c_reference/test_practrand_32bit_out.txt) |
| 64-bit  | 192 bits  | [did not fail, tested to 2^45 bytes (32 TB)](https://github.com/danielcota/biski64_dev/blob/main/c_reference/test_practrand_64bit_out.txt) |
		
The results for the 8-bit and 16-bit scaled down versions show that `biski64` exceeds the mixing efficiency (in terms of PractRand bytes passed per total state size) of even the [well respected and tested JSF PRNG](https://www.pcg-random.org/posts/bob-jenkins-small-prng-passes-practrand.html).

For comparison:

| PRNG  | Total State | Practrand Failure | Ratio (failed exponent/state bits) |
| ------------- | ------------- | ------------- | ------------- |
| biski(8-bit)  | 24 bits | 2^22 bytes| **91.7%** |
| jsf8  | 32 bits | 2^28 bytes | 87.5% |

| PRNG  | Total State | Practrand Failure | Ratio (failed exponent/state bits) |
| ------------- | ------------- | ------------- | ------------- |
| biski(16-bit)  | 48 bits | 2^40 bytes| **83.3%** |
| jsf16  | 64 bits | 2^47 bytes | 73.4% |

The scaled down test results for `biski64` indicate strong performance of its core mixing algorithm (suggesting that the strong performance at full 64-bit size is not a result simply of a large state, but of its robust mixing performance).



## BigCrush Comparison

BigCrush was run 100 times on `biski64` (and multiple reference PRNGs).

Assuming test failure with a p-value below 0.001 or above 0.999 implies a 0.2% probability of a single test yielding a "false positive" purely by chance with an ideal generator. Running BigCrush 100 times (for a total of 25400 sub-tests), around 50.8 such chance "errors" would be anticipated.

```
biski64, 51 failed subtests (out of 25400 total)
  5 subtests failed twice

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


## Design

Motivated by M.E. O'Neill's post, [Does It Beat the Minimal Standard](https://www.pcg-random.org/posts/does-it-beat-the-minimal-standard.html) - the initial design for `biski64` used a scaled down version with 8-bit state variables.  This allowed for fast iteration using PractRand.

Once the algorithm was settled on, parameters were selected as follows:
1. **Additive Constant** - Multiple additive constants were tested using a 16-bit state variable version.  For each constant all possible rotation constants r1 and r2 (r1 < r2) were iterated over and PractRand was run to determine how far each (additiveConstant, r1, r2) variant could get.  The highest performing constant was 0x9999 - passing the most rotational pairs at the highest level.  This constant was then scaled to 0x9999999999999999 in the full 64-bit version.
2. **Rotation Constants** - It was observed that bitsPerStateVar / 4 showed up frequently at the highest level for r1, as well as integers near bitsPerStateVar * (ɸ - 1) for r2.  This was then used as a heuristic for rotation constants in the 32-bit and 64-bit versions.
3. **BigCrush Confirmation** - BigCrush was run 100 times on five selected full size 64-bit variants (using the above heuristics).  The best observed r2 values were 39 and 40 - supporting the theory that r1=bits/4 and r2=bits * (ɸ - 1) were solid heuristics. r2=40 was then deemed the best (as it exhibited no subtests failing three times) - and then used for the full 64-bit version.

```
(r1=16, r2=37) 64 subtests failed (out of 25400 total)
   2 subtests failed THREE times
   7 subtests failed twice

(r1=16, r2=39) 46 subtests failed (out of 25400 total)
   1 subtest failed THREE times
   4 subtests failed twice

(r1=16, r2=40) 51 subtests failed (out of 25400 total)
   5 subtests failed twice

(r1=16, r2=41) 60 subtests failed (out of 25400 total)
   1 subtest failed THREE times
   6 subtests failed twice

(r1=16, r2=43) 46 subtests failed (out of 25400 total)
   1 subtest failed FOUR times
   1 subtest failed THREE times
   1 subtest failed twice
```


## Notes
Created by Daniel Cota and named after his cat Biscuit - a small and fast Egyptian Mau.
