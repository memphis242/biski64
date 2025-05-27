use biski64::Biski64Rng;
use rand_core::{RngCore, SeedableRng};

fn main() {
    // Create a new generator instance from a seed.
    let mut rng = Biski64Rng::seed_from_u64(12345);

    println!("Generating 5 random numbers:");
    for i in 1..=5 {
        // Call the next_u64() method from the RngCore trait.
        let num = rng.next_u64();
        println!("  {}: {}", i, num);
    }
}
