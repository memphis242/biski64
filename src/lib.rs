//! biski64: A fast, 64-bit PRNG that implements the rand_core traits.
//!
//! This library provides `Biski64Rng`, a fast, robust, non-cryptographic PRNG
//! with a guaranteed minimum period of 2^64. It is designed for applications where
//! speed and statistical quality are important.

#![no_std]

use rand_core::{RngCore, SeedableRng};

// Helper struct for seeding. This is a complete SplitMix64 PRNG.
struct SplitMix64 {
    state: u64,
}

impl SplitMix64 {
    fn new(seed: u64) -> Self {
        Self { state: seed }
    }

    fn next(&mut self) -> u64 {
        self.state = self.state.wrapping_add(0x9e3779b97f4a7c15);
        let mut z = self.state;
        z = (z ^ (z >> 30)).wrapping_mul(0xbf58476d1ce4e5b9);
        z = (z ^ (z >> 27)).wrapping_mul(0x94d049bb133111eb);
        z ^ (z >> 31)
    }
}

/// An instance of the `biski64` pseudo-random number generator.
///
/// `Biski64Rng` is a fast, robust, non-cryptographic PRNG with a guaranteed
/// minimum period of 2^64. It is designed for applications where speed and statistical
/// quality are important.
///
/// This generator implements the `RngCore` and `SeedableRng` traits from the
/// `rand_core` crate, allowing it to be used as a drop-in replacement for
/// other PRNGs in the Rust ecosystem.
///
/// # Example
/// ```
/// use biski64::Biski64Rng;
/// use rand_core::{RngCore, SeedableRng};
///
/// // Create a new generator from a seed
/// let mut rng = Biski64Rng::seed_from_u64(42);
///
/// // Generate a random u64 number
/// let num = rng.next_u64();
/// ```
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Biski64Rng {
    fast_loop: u64,
    mix: u64,
    loop_mix: u64,
}

impl Biski64Rng {
    /// Creates a new `Biski64Rng` for a specific stream in a parallel setup.
    ///
    /// This function initializes the generator's state to a unique, non-overlapping
    /// sequence based on its stream index. It is the recommended way to create
    /// multiple PRNGs for parallel tasks that all originate from a single base seed.
    ///
    /// # Arguments
    /// * `seed`: The master seed for all streams.
    /// * `stream_index`: The index of this stream (from 0 to `total_streams` - 1).
    /// * `total_streams`: The total number of streams being created.
    ///
    /// # Panics
    /// Panics if `total_streams` is 0 or if `stream_index` is not less than `total_streams`.
    pub fn from_seed_for_stream(seed: u64, stream_index: u64, total_streams: u64) -> Self {
        assert!(total_streams >= 1, "Total number of streams must be at least 1.");
        assert!(stream_index < total_streams, "Stream index must be less than total streams.");

        let mut seeder = SplitMix64::new(seed);

        // Generate the initial state from the seeder.
        let mut s0 = 0;
        let mut s1 = 0;
        let mut s2 = 0;
        while s0 == 0 && s1 == 0 && s2 == 0 {
            s0 = seeder.next(); // Becomes the base for fast_loop
            s1 = seeder.next(); // Becomes mix
            s2 = seeder.next(); // Becomes loop_mix
        }

        let base_fast_loop = s0;

        let final_fast_loop = if total_streams > 1 {
            // Use u128 for intermediate calculations to prevent overflow.
            let cycles_per_stream: u128 = (u64::MAX as u128) / (total_streams as u128);

            // Calculate the offset for this stream's sequence.
            let offset: u128 = (stream_index as u128)
                .wrapping_mul(cycles_per_stream)
                .wrapping_mul(0x9999999999999999);

            // Add the offset to the random base value. The conversion from u128 to u64
            // and the `wrapping_add` correctly simulate the desired modular arithmetic.
            base_fast_loop.wrapping_add(offset as u64)
        } else {
            base_fast_loop
        };

        let mut rng = Self {
            fast_loop: final_fast_loop,
            mix: s1,
            loop_mix: s2,
        };

        // Warm-up period.
        for _ in 0..16 {
            rng.next_u64();
        }

        rng
    }
}

impl RngCore for Biski64Rng {
    #[inline(always)]
    fn next_u64(&mut self) -> u64 {
        let output = self.mix.wrapping_add(self.loop_mix);

        (self.fast_loop, self.mix, self.loop_mix) = (
            self.fast_loop.wrapping_add(0x9999999999999999),
            self.mix.rotate_left(16).wrapping_add(self.loop_mix.rotate_left(40)),
            self.fast_loop ^ self.mix,
        );

        output
    }

    #[inline(always)]
    fn next_u32(&mut self) -> u32 {
        (self.next_u64() >> 32) as u32
    }

    fn fill_bytes(&mut self, dest: &mut [u8]) {
        rand_core::impls::fill_bytes_via_next(self, dest)
    }
}

impl SeedableRng for Biski64Rng {
    type Seed = [u8; 32];

    /// Creates a new `Biski64Rng` from a 32-byte seed.
    ///
    /// This implementation uses a `SplitMix64` generator to initialize the state,
    /// ensuring a robust and well-distributed starting state from the given seed.
    /// A 16-iteration warm-up period is then applied.
    fn from_seed(seed: Self::Seed) -> Self {
        // Use the first 8 bytes of the seed for our seeder.
        let seed_for_seeder = u64::from_le_bytes(seed[0..8].try_into().unwrap());

        // Create a SplitMix64 instance to generate our actual state.
        let mut seeder = SplitMix64::new(seed_for_seeder);

        // Ensure the initial state is not all zero, which can be a bad state.
        let mut s0 = 0;
        let mut s1 = 0;
        let mut s2 = 0;
        while s0 == 0 && s1 == 0 && s2 == 0 {
            s0 = seeder.next();
            s1 = seeder.next();
            s2 = seeder.next();
        }

        // Construct the initial RNG instance
        let mut rng = Self {
            fast_loop: s0,
            mix: s1,
            loop_mix: s2,
        };

        // Warm-up period: 16 iterations to further diffuse the initial state.
        // This helps ensure that the initial outputs are well mixed.
        for _ in 0..16 {
            rng.next_u64();
        }

        // Return the warmed-up RNG
        rng
    }
}


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_sequence_from_known_seed() {
        // Test that a known seed produces a known, repeatable sequence.
        // This prevents regressions in the algorithm.
        let mut rng = Biski64Rng::seed_from_u64(12345);
        assert_eq!(rng.next_u64(), 11653916018131561298);
        assert_eq!(rng.next_u64(), 8879211503557437945);
        assert_eq!(rng.next_u64(), 8034477089638237744);
    }

    #[test]
    fn test_seed_from_u64_is_deterministic() {
        let mut rng1 = Biski64Rng::seed_from_u64(42);
        let mut rng2 = Biski64Rng::seed_from_u64(42);
        // This WILL pass, as it uses the same function with the same input.
        assert_eq!(rng1.next_u64(), rng2.next_u64());
    }

    #[test]
    fn test_from_seed_is_deterministic() {
        // It's good practice to use a non-trivial seed for testing.
        let mut seed = [0u8; 32];
        seed[0] = 1;
        seed[5] = 5;
        seed[15] = 15;
        seed[31] = 31;

        let mut rng1 = Biski64Rng::from_seed(seed);
        let mut rng2 = Biski64Rng::from_seed(seed);
        // This WILL pass, as it uses the same function with the same input.
        assert_eq!(rng1.next_u64(), rng2.next_u64());
    }

    #[test]
    fn test_parallel_streams_start_differently() {
        let seed = 123456789;
        let mut stream0 = Biski64Rng::from_seed_for_stream(seed, 0, 4);
        let mut stream1 = Biski64Rng::from_seed_for_stream(seed, 1, 4);
        let mut stream2 = Biski64Rng::from_seed_for_stream(seed, 2, 4);

        // With the same seed, different stream indices must produce different starting values.
        let val0 = stream0.next_u64();
        let val1 = stream1.next_u64();
        let val2 = stream2.next_u64();

        assert_ne!(val0, val1, "Streams 0 and 1 should not produce the same first value");
        assert_ne!(val0, val2, "Streams 0 and 2 should not produce the same first value");
        assert_ne!(val1, val2, "Streams 1 and 2 should not produce the same first value");
    }
}
